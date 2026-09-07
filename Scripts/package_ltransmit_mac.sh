#!/bin/bash
# Package the formal map and copy the complete staged Mac app (including UE content).
set -euo pipefail
repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
engine_dir="${TRANSMIT_ENGINE_DIR:-/Users/Shared/Epic Games/UE_5.8}"
candidate_dir="${1:-$repo_dir/Saved/LTransmitCandidate/$(date +%Y%m%d-%H%M%S)}"
if [[ -e "$candidate_dir/Transmit.app" ]]; then
    echo "Refusing to overwrite an existing candidate: $candidate_dir/Transmit.app" >&2
    exit 1
fi
mkdir -p "$candidate_dir"
"$engine_dir/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
    "-project=$repo_dir/passely.uproject" -noP4 -platform=Mac \
    -clientconfig=Development -build -cook -stage -pak -iostore \
    -map=/Game/Transmit/Maps/L_Transmit -AdditionalCookerOptions=-SkipZenStore \
    -unattended -utf8output 2>&1 | tee "$candidate_dir/package.log"
staged_app="$repo_dir/Saved/StagedBuilds/Mac/passely.app"
test -x "$staged_app/Contents/MacOS/passely"
test -d "$staged_app/Contents/UE/passely/Content/Paks"
ditto "$staged_app" "$candidate_dir/Transmit.app"
codesign --verify --deep --strict "$candidate_dir/Transmit.app"
git -C "$repo_dir" rev-parse HEAD > "$candidate_dir/source-head.txt"
git -C "$repo_dir" status --short > "$candidate_dir/source-status.txt"
echo "Candidate: $candidate_dir/Transmit.app"
