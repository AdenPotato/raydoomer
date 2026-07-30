#include "platform/raylib/raylib_renderer.h"

#include "platform/violation.h"

#include <raylib.h>
#include <raymath.h>

#include <string>
#include <vector>

namespace platform {
namespace {

// World positions are double; raylib takes float. The narrowing happens here, at
// the last possible moment, so precision is only lost on the way to the GPU
// rather than anywhere it could affect the simulation.
::Vector3 toRaylib(Point3 p) {
    return ::Vector3{ static_cast<float>(p.x), static_cast<float>(p.y),
                      static_cast<float>(p.z) };
}

::Vector3 toRaylib(Vec3 v) {
    return ::Vector3{ v.x, v.y, v.z };
}

::Color toRaylib(Color c) {
    return ::Color{ c.r, c.g, c.b, c.a };
}

constexpr int kSphereRings = 8;
constexpr int kSphereSlices = 8;

} // namespace

struct RaylibRenderer::Impl {
    template <typename Resource>
    struct Slot {
        Resource resource{};
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Slot<::Mesh>> meshes;
    std::vector<uint32_t> freeMeshes;
    std::vector<Slot<::Texture2D>> textures;
    std::vector<uint32_t> freeTextures;

    // One material reused for every mesh draw. raylib's DrawMesh needs a
    // material, and allocating one per draw would allocate in the hot path.
    ::Material material{};
    bool materialLoaded = false;

    ViolationReporter onViolation;
    Camera activeCamera{};

    void report(const std::string& message) const {
        if (onViolation) {
            onViolation(message);
        }
    }
};

RaylibRenderer::RaylibRenderer(ViolationReporter onViolation)
    : impl_(std::make_unique<Impl>()) {
    impl_->onViolation = std::move(onViolation);
    impl_->material = LoadMaterialDefault();
    impl_->materialLoaded = true;
}

RaylibRenderer::~RaylibRenderer() {
    // Every acquire has a matching release, including the ones a caller forgot.
    for (auto& slot : impl_->meshes) {
        if (slot.alive) {
            UnloadMesh(slot.resource);
        }
    }
    for (auto& slot : impl_->textures) {
        if (slot.alive) {
            UnloadTexture(slot.resource);
        }
    }
    if (impl_->materialLoaded) {
        // The default material's shader is owned by raylib itself, so only the
        // material struct is released here.
        impl_->material.maps = nullptr;
    }
}

MeshHandle RaylibRenderer::createMesh(std::span<const Vertex> vertices,
                                      std::span<const uint16_t> indices) {
    if (vertices.empty() || indices.empty()) {
        impl_->report("createMesh called with no geometry");
        return MeshHandle{};
    }

    ::Mesh mesh{};
    mesh.vertexCount = static_cast<int>(vertices.size());
    mesh.triangleCount = static_cast<int>(indices.size() / 3);

    // raylib takes ownership of these and frees them in UnloadMesh.
    mesh.vertices = static_cast<float*>(MemAlloc(vertices.size() * 3 * sizeof(float)));
    mesh.normals = static_cast<float*>(MemAlloc(vertices.size() * 3 * sizeof(float)));
    mesh.texcoords = static_cast<float*>(MemAlloc(vertices.size() * 2 * sizeof(float)));
    mesh.indices = static_cast<unsigned short*>(
        MemAlloc(indices.size() * sizeof(unsigned short)));

    for (size_t i = 0; i < vertices.size(); ++i) {
        mesh.vertices[i * 3 + 0] = vertices[i].position.x;
        mesh.vertices[i * 3 + 1] = vertices[i].position.y;
        mesh.vertices[i * 3 + 2] = vertices[i].position.z;
        mesh.normals[i * 3 + 0] = vertices[i].normal.x;
        mesh.normals[i * 3 + 1] = vertices[i].normal.y;
        mesh.normals[i * 3 + 2] = vertices[i].normal.z;
        mesh.texcoords[i * 2 + 0] = vertices[i].uv.x;
        mesh.texcoords[i * 2 + 1] = vertices[i].uv.y;
    }
    for (size_t i = 0; i < indices.size(); ++i) {
        mesh.indices[i] = indices[i];
    }

    // Uploaded once, drawn many times. This is the whole reason the mesh path
    // exists rather than re-specifying geometry every frame.
    UploadMesh(&mesh, /*dynamic=*/false);

    uint32_t index = 0;
    if (!impl_->freeMeshes.empty()) {
        index = impl_->freeMeshes.back();
        impl_->freeMeshes.pop_back();
    } else {
        index = static_cast<uint32_t>(impl_->meshes.size());
        impl_->meshes.emplace_back();
    }

    auto& slot = impl_->meshes[index];
    slot.resource = mesh;
    slot.alive = true;
    slot.generation += 1; // starts at 1, so a default handle is never valid
    return MeshHandle{ index, slot.generation };
}

void RaylibRenderer::destroyMesh(MeshHandle mesh) {
    if (mesh.generation == 0) {
        return; // destroying nothing is harmless
    }
    if (mesh.index >= impl_->meshes.size() ||
        impl_->meshes[mesh.index].generation != mesh.generation ||
        !impl_->meshes[mesh.index].alive) {
        impl_->report("stale mesh handle passed to destroyMesh");
        return;
    }
    UnloadMesh(impl_->meshes[mesh.index].resource);
    impl_->meshes[mesh.index].alive = false;
    impl_->freeMeshes.push_back(mesh.index);
}

TextureHandle RaylibRenderer::createTexture(int width, int height,
                                            std::span<const uint8_t> pixels) {
    const size_t expected = static_cast<size_t>(width) * height * 4;
    if (width <= 0 || height <= 0 || pixels.size() < expected) {
        impl_->report("createTexture called with mismatched dimensions and pixel data");
        return TextureHandle{};
    }

    ::Image image{};
    image.data = const_cast<uint8_t*>(pixels.data()); // LoadTextureFromImage copies
    image.width = width;
    image.height = height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    const ::Texture2D texture = LoadTextureFromImage(image);

    uint32_t index = 0;
    if (!impl_->freeTextures.empty()) {
        index = impl_->freeTextures.back();
        impl_->freeTextures.pop_back();
    } else {
        index = static_cast<uint32_t>(impl_->textures.size());
        impl_->textures.emplace_back();
    }

    auto& slot = impl_->textures[index];
    slot.resource = texture;
    slot.alive = true;
    slot.generation += 1;
    return TextureHandle{ index, slot.generation };
}

void RaylibRenderer::destroyTexture(TextureHandle texture) {
    if (texture.generation == 0) {
        return;
    }
    if (texture.index >= impl_->textures.size() ||
        impl_->textures[texture.index].generation != texture.generation ||
        !impl_->textures[texture.index].alive) {
        impl_->report("stale texture handle passed to destroyTexture");
        return;
    }
    UnloadTexture(impl_->textures[texture.index].resource);
    impl_->textures[texture.index].alive = false;
    impl_->freeTextures.push_back(texture.index);
}

void RaylibRenderer::beginScene(const Camera& camera) {
    impl_->activeCamera = camera;

    ::Camera3D raylibCamera{};
    raylibCamera.position = toRaylib(camera.position);
    raylibCamera.target = toRaylib(camera.target);
    raylibCamera.up = ::Vector3{ 0.0f, 1.0f, 0.0f };
    raylibCamera.fovy = camera.fovDegrees;
    raylibCamera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(raylibCamera);
    // A ground grid gives wireframes a sense of scale and orientation they
    // otherwise completely lack.
    DrawGrid(40, 1.0f);
}

void RaylibRenderer::endScene() {
    EndMode3D();
}

void RaylibRenderer::drawMesh(MeshHandle mesh, const Transform& transform,
                              TextureHandle texture, Color tint) {
    if (mesh.generation == 0 || mesh.index >= impl_->meshes.size() ||
        impl_->meshes[mesh.index].generation != mesh.generation ||
        !impl_->meshes[mesh.index].alive) {
        impl_->report("stale mesh handle passed to drawMesh");
        return;
    }

    if (texture.generation != 0 && texture.index < impl_->textures.size() &&
        impl_->textures[texture.index].alive) {
        impl_->material.maps[MATERIAL_MAP_DIFFUSE].texture =
            impl_->textures[texture.index].resource;
    } else {
        impl_->material.maps[MATERIAL_MAP_DIFFUSE].texture = ::Texture2D{};
    }
    impl_->material.maps[MATERIAL_MAP_DIFFUSE].color = toRaylib(tint);

    const ::Matrix translation = MatrixTranslate(static_cast<float>(transform.position.x),
                                                 static_cast<float>(transform.position.y),
                                                 static_cast<float>(transform.position.z));
    const ::Matrix rotation = QuaternionToMatrix(::Quaternion{
        transform.rotation.x, transform.rotation.y, transform.rotation.z,
        transform.rotation.w });
    const ::Matrix scale =
        MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);

    DrawMesh(impl_->meshes[mesh.index].resource, impl_->material,
             MatrixMultiply(MatrixMultiply(scale, rotation), translation));
}

void RaylibRenderer::drawSprite(TextureHandle texture, Point3 position, Vec2 size,
                                Rect uvRect, Color tint) {
    if (texture.generation == 0 || texture.index >= impl_->textures.size() ||
        !impl_->textures[texture.index].alive) {
        impl_->report("stale texture handle passed to drawSprite");
        return;
    }

    const ::Texture2D& raylibTexture = impl_->textures[texture.index].resource;

    // UVs are normalised at the boundary; raylib wants pixels.
    const ::Rectangle source{ uvRect.x * static_cast<float>(raylibTexture.width),
                              uvRect.y * static_cast<float>(raylibTexture.height),
                              uvRect.width * static_cast<float>(raylibTexture.width),
                              uvRect.height * static_cast<float>(raylibTexture.height) };

    ::Camera3D raylibCamera{};
    raylibCamera.position = toRaylib(impl_->activeCamera.position);
    raylibCamera.target = toRaylib(impl_->activeCamera.target);
    raylibCamera.up = ::Vector3{ 0.0f, 1.0f, 0.0f };
    raylibCamera.fovy = impl_->activeCamera.fovDegrees;
    raylibCamera.projection = CAMERA_PERSPECTIVE;

    // raylib orients the billboard itself from the camera. The equivalent maths
    // lives in engine/billboard.h so it can be tested without a device; this
    // path uses raylib's because it also handles the projection.
    DrawBillboardRec(raylibCamera, raylibTexture, source, toRaylib(position),
                     ::Vector2{ size.x, size.y }, toRaylib(tint));
}

void RaylibRenderer::drawWireBox(Point3 center, Vec3 halfExtents, Color color) {
    DrawCubeWires(toRaylib(center), halfExtents.x * 2.0f, halfExtents.y * 2.0f,
                  halfExtents.z * 2.0f, toRaylib(color));
}

void RaylibRenderer::drawWireSphere(Point3 center, float radius, Color color) {
    DrawSphereWires(toRaylib(center), radius, kSphereRings, kSphereSlices, toRaylib(color));
}

void RaylibRenderer::drawSolidBox(Point3 center, Vec3 halfExtents, Color color) {
    DrawCube(toRaylib(center), halfExtents.x * 2.0f, halfExtents.y * 2.0f,
             halfExtents.z * 2.0f, toRaylib(color));
}

void RaylibRenderer::drawSolidSphere(Point3 center, float radius, Color color) {
    DrawSphere(toRaylib(center), radius, toRaylib(color));
}

void RaylibRenderer::drawLine(Point3 from, Point3 to, Color color) {
    DrawLine3D(toRaylib(from), toRaylib(to), toRaylib(color));
}

void RaylibRenderer::drawPoint(Point3 position, float size, Color color) {
    // raylib has no 3D point primitive, so a small sphere stands in. Clamped
    // because Box3D asks for some marker sizes in screen pixels, which would be
    // enormous interpreted as metres.
    constexpr float kMaxWorldSize = 0.15f;
    const float radius = size > kMaxWorldSize ? kMaxWorldSize : size;
    DrawSphere(toRaylib(position), radius, toRaylib(color));
}

void RaylibRenderer::begin2D() {
    // raylib draws 2D in screen space by default, so no state change is needed.
    // The scope still exists at the interface: a backend that does need one can
    // implement it, and callers already bracket their overlay correctly.
}

void RaylibRenderer::end2D() {}

Vec2 RaylibRenderer::viewportSize() const {
    return Vec2{ static_cast<float>(GetScreenWidth()),
                 static_cast<float>(GetScreenHeight()) };
}

void RaylibRenderer::drawRect(Rect area, Color color) {
    DrawRectangle(static_cast<int>(area.x), static_cast<int>(area.y),
                  static_cast<int>(area.width), static_cast<int>(area.height),
                  toRaylib(color));
}

void RaylibRenderer::drawTexturedRect(TextureHandle texture, Rect area, Rect uvRect,
                                      Color tint) {
    if (texture.generation == 0 || texture.index >= impl_->textures.size() ||
        !impl_->textures[texture.index].alive) {
        impl_->report("stale texture handle passed to drawTexturedRect");
        return;
    }

    const ::Texture2D& raylibTexture = impl_->textures[texture.index].resource;
    const ::Rectangle source{ uvRect.x * static_cast<float>(raylibTexture.width),
                              uvRect.y * static_cast<float>(raylibTexture.height),
                              uvRect.width * static_cast<float>(raylibTexture.width),
                              uvRect.height * static_cast<float>(raylibTexture.height) };
    const ::Rectangle destination{ area.x, area.y, area.width, area.height };

    DrawTexturePro(raylibTexture, source, destination, ::Vector2{ 0.0f, 0.0f }, 0.0f,
                   toRaylib(tint));
}

void RaylibRenderer::drawText(std::string_view text, Vec2 position, float size,
                              Color color) {
    // raylib's DrawText needs a null-terminated string; string_view is not.
    const std::string terminated(text);
    DrawText(terminated.c_str(), static_cast<int>(position.x),
             static_cast<int>(position.y), static_cast<int>(size), toRaylib(color));
}

} // namespace platform
