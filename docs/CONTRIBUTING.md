# Contributing

Thanks for wanting to contribute! This file explains the preferred workflow, pull request etiquette, and the commit message format we require (Conventional Commits).

## Workflow

- Create a branch for each change/task: `git checkout -b feat/short-description` or `git checkout -b fix/issue-123`.
- Make small, focused commits. Keep PRs focused to one main change.
- Open a Pull Request against `main`. In the PR description, reference related issues (e.g. `Closes #42`).
- Ensure code builds and any relevant tests pass locally before opening the PR.

## Code style and formatting

- Follow existing project style. Keep changes minimal and targeted.
- Add or update tests if you introduce or change behavior.

## Commit message format — Conventional Commits (required)

We use Conventional Commits to keep history readable and to enable automated changelogs.

Format:

```
<type>(<scope>): <short summary>

<optional body>

<optional footer(s)>
```

Where:
- type: one of `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`, `build`, `ci`, `revert`
- scope: optional area of the codebase (e.g. `renderer`, `lua`, `build`)
- short summary: 50 characters or less preferred, imperative, no trailing period

Examples:

```
feat(renderer): add OpenGL 4.5 fallback support

fix(physic): prevent bullets from passing through thin colliders

docs: update README with build instructions for Windows
```

Breaking changes
- When a commit introduces a breaking change include `BREAKING CHANGE:` in the body or footer and describe the change.

Referencing issues
- Use `Closes #<issue-number>` in the footer to automatically close issues when the commit/PR is merged.

Tools (optional)
- You can use tools like Commitizen, husky, or git hooks to help enforce Conventional Commits, but they are not required. Example local commit command:

```bash
git commit -m "feat(renderer): support HDR output"
```

## Pull request template (recommended)

- Title: use a short descriptive title — prefer the same style as commit subject.
- Description: What changed and why. Link to issues. Include testing steps and screenshots if applicable.
- Checklist (example):
  - [ ] Builds on my machine (Linux/Windows)
  - [ ] No new warnings
  - [ ] Relevant tests added/updated

## Review & merge

- Maintainters will review and request changes if necessary.
- Squash or rebase commits as requested by reviewers. The repository prefers a clean history, and maintainers may squash on merge depending on the project policy.

Thank you for contributing — your help makes the project better! 🎮
