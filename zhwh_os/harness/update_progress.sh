#!/bin/bash
# OS Development Harness - Progress Updater
#
# Updates the progress of a feature and commits changes
#
# Usage: ./update_progress.sh <FEATURE_ID> <passing|failing> [commit_message]

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Get session directory
SESSION_DIR=$(ls -t harness/sessions/ | head -1)

if [ -z "$SESSION_DIR" ]; then
    echo "Error: No active session found. Run ./harness/init_session.sh first."
    exit 1
fi

SESSION_PATH="harness/sessions/$SESSION_DIR"

if [ ! -d "$SESSION_PATH" ]; then
    echo "Error: Session $SESSION_DIR not found"
    exit 1
fi

FEATURE_ID=${1}
FEATURE_STATUS=${2}
COMMIT_MSG=${3:-"Update $FEATURE_ID to $FEATURE_STATUS"}

if [ -z "$FEATURE_ID" ] || [z "$FEATURE_STATUS" ]; then
    echo "Usage: ./update_progress.sh <feature_id> <passing|failing> [commit_message]"
    echo ""
    echo "Example:"
    echo "  ./update_progress.sh SI-001 passing \"Implement syscall entry/exit\""
    exit 1
fi

# Validate status
if [ "$FEATURE_STATUS" != "passing" ] && [ "$FEATURE_STATUS" != "failing" ]; then
    echo "Error: Status must be 'passing' or 'failing'"
    exit 1
fi

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}Updating Feature Progress${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""
echo "Feature: $FEATURE_ID"
echo "Status: $FEATURE_STATUS"
echo "Session: $SESSION_DIR"
echo ""

# Check if feature exists in feature_list.json
if ! grep -q "\"$FEATURE_ID\"" harness/feature_list.json; then
    echo "Warning: Feature $FEATURE_ID not found in feature_list.json"
    echo "Continuing anyway..."
fi

# Update feature_list.json
echo -e "${GREEN}[1/4] Updating feature_list.json${NC}"

# Create a Python script to update JSON
cat > /tmp/update_feature.py <<'PYTHON'
import json
import sys

feature_id = sys.argv[1]
status = sys.argv[2]

try:
    with open('harness/feature_list.json', 'r') as f:
        data = json.load(f)

    # Find and update the feature
    found = False
    for cat_name, cat_data in data['categories'].items():
        for feature in cat_data['features']:
            if feature['id'] == feature_id:
                old_status = feature['status']
                feature['status'] = status
                found = True
                print(f"Updated {feature_id}: {feature['name']}")
                print(f"  {old_status} -> {status}")
                break
        if found:
            break

    if not found:
        print(f"Warning: Feature {feature_id} not found")
        sys.exit(1)

    # Update counts
    total = 0
    passing = 0
    for cat_name, cat_data in data['categories'].items():
        for feature in cat_data['features']:
            total += 1
            if feature['status'] == 'passing':
                passing += 1

    data['total_features'] = total
    data['completed_features'] = passing
    data['last_updated'] = "2026-03-03"

    with open('harness/feature_list.json', 'w') as f:
        json.dump(data, f, indent=2)

    print(f"\nProgress: {passing}/{total} features passing ({passing*100//total}%)")

except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
PYTHON

python3 /tmp/update_feature.py "$FEATURE_ID" "$FEATURE_STATUS"

echo ""
echo -e "${GREEN}[2/4] Updating session manifest${NC}"

# Update session manifest
cat > /tmp/update_manifest.py <<'PYTHON'
import json
import sys
from datetime import datetime

session_path = sys.argv[1]
feature_id = sys.argv[2]
status = sys.argv[3]

try:
    with open(f'{session_path}/manifest.json', 'r') as f:
        manifest = json.load(f)

    if 'features_worked' not in manifest:
        manifest['features_worked'] = []

    # Add feature entry
    manifest['features_worked'].append({
        'id': feature_id,
        'status': status,
        'time': datetime.now().isoformat()
    })

    manifest['last_update'] = datetime.now().isoformat()

    with open(f'{session_path}/manifest.json', 'w') as f:
        json.dump(manifest, f, indent=2)

    print("Session manifest updated")

except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
PYTHON

python3 /tmp/update_manifest.py "$SESSION_PATH" "$FEATURE_ID" "$FEATURE_STATUS"

echo ""
echo -e "${GREEN}[3/4] Updating progress log${NC}"

# Add to progress log
echo "## $(date)" >> "$SESSION_PATH/progress.log"
echo "- Updated $FEATURE_ID: $FEATURE_STATUS" >> "$SESSION_PATH/progress.log"
echo "- Commit: $COMMIT_MSG" >> "$SESSION_PATH/progress.log"
echo "" >> "$SESSION_PATH/progress.log"

echo "Progress log updated"

echo ""
echo -e "${GREEN}[4/4] Git Commit${NC}"

# Commit changes
cd "$(dirname "$0")/../.."

# Check if there are changes to commit
if [ -n "$(git status --porcelain)" ]; then
    git add -A
    git commit -m "$COMMIT_MSG" -m ""
    git commit -m "Feature: $FEATURE_ID" -m "Status: $FEATURE_STATUS" -m "Session: $SESSION_DIR"
    echo -e "${GREEN}✓${NC} Changes committed"
    echo ""
    git log -1 --oneline
else
    echo "No changes to commit"
fi

echo ""
echo -e "${BLUE}======================================${NC}"
echo -e "${GREEN}Progress Update Complete!${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""
echo "Feature $FEATURE_ID marked as $FEATURE_STATUS"
echo "Session: $SESSION_DIR"
echo ""
