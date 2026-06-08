#!/usr/bin/env bash
# Cut a new release: tag HEAD and push — GitHub Actions builds and publishes it.
#
# Usage:  ./release.sh 1.2.0
#         ./release.sh v1.2.0   (leading v is accepted)
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <version>   e.g. $0 1.2.0"
    exit 1
fi

VERSION="${1#v}"    # strip leading v if provided
TAG="v${VERSION}"

# Must be on master
BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [[ "$BRANCH" != "master" ]]; then
    echo "Error: must be on master (currently on '$BRANCH')."
    exit 1
fi

# Working tree must be clean
if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Error: working tree is dirty — commit or stash changes first."
    exit 1
fi

# Tag must not already exist
if git rev-parse "$TAG" &>/dev/null; then
    echo "Error: tag '$TAG' already exists."
    exit 1
fi

git tag -a "$TAG" -m "Release $TAG"
git push origin "$TAG"

echo ""
echo "Tag $TAG pushed to origin."
echo "GitHub Actions will build all platforms and publish the release."
if command -v gh &>/dev/null; then
    REPO_URL=$(gh repo view --json url -q .url 2>/dev/null || true)
    [[ -n "$REPO_URL" ]] && echo "Track progress: ${REPO_URL}/actions"
fi
