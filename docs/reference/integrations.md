# External Service Integrations (template)

Developer runbook for setting up every external service the project depends on. Follow this in order when provisioning a new environment.

This is a **template**. Replace the example service with your project's real services - one section each. Keep the per-service shape: **Purpose -> Setup -> Environment variables -> Verification**.

---

## Services

| Service     | Purpose                          | Environments  |
|-------------|----------------------------------|---------------|
| `<service>` | `<what it does for the project>` | `<dev, prod>` |

---

## `<Service name>`

**Purpose:** `<one or two sentences: what this service provides and where it is used.>`

**Setup**

1. `<account / project creation step>`
2. `<configuration step>`
3. `<credentials step - where keys come from>`
4. `<any environment-specific notes (dev sandbox vs prod)>`

**Environment variables**

| Variable    | Value             | Used by       |
|-------------|-------------------|---------------|
| `<ENV_VAR>` | `<what it holds>` | `<which app>` |

> Document every new env var (name + one-line description) before the PR. Never commit real keys.

**Verification:** `<the concrete check that proves the integration works - a command to run, an endpoint to hit, or a dashboard entry to confirm.>`

---

<!-- Duplicate the service block above for each additional external service. -->
