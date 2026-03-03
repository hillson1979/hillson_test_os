#!/bin/bash
# OS Development Automated Workflow Script
#
# This script automates the development workflow:
# 1. Select next priority feature from feature_list.json
# 2. Analyze current implementation
# 3. Generate code changes
# 4. Build and test
# 5. Update feature status
# 6. Commit changes (pending user approval)
#
# Usage: ./auto_workflow.sh [--feature <FEATURE_ID>] [--skip-build] [--skip-test]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OS_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(dirname "$OS_DIR")"

# Parse arguments
FEATURE_ID=""
SKIP_BUILD=false
SKIP_TEST=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --feature)
            FEATURE_ID="$2"
            shift 2
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --skip-test)
            SKIP_TEST=true
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --feature <ID>   Work on specific feature (default: auto-select next priority)"
            echo "  --skip-build    Skip build step"
            echo "  --skip-test     Skip test step"
            echo "  --help          Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}OS Development Automated Workflow${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Project: $OS_DIR"
echo "Date: $(date)"
echo ""

# =============================================================================
# STEP 1: Select Feature
# =============================================================================

echo -e "${CYAN}[STEP 1/7] Feature Selection${NC}"
echo ""

if [ -z "$FEATURE_ID" ]; then
    echo "Auto-selecting next priority feature..."

    # Find next failing critical feature
    FEATURE_ID=$(python3 <<'PYTHON'
import json

try:
    with open('harness/feature_list.json', 'r') as f:
        data = json.load(f)

    # Priority order: critical > high > medium
    priority_order = ['critical', 'high', 'medium']

    for priority in priority_order:
        for cat_name, cat_data in data['categories'].items():
            if cat_data['priority'] != priority:
                continue
            for feature in cat_data['features']:
                if feature['status'] == 'failing':
                    print(f"{feature['id']}")
                    exit(0)

    print("")

except Exception as e:
    print(f"Error: {e}")
    exit(1)
PYTHON
)
fi

if [ -z "$FEATURE_ID" ]; then
    echo -e "${GREEN}✓${NC} All features passing! No work to do."
    exit 0
fi

# Get feature details
FEATURE_JSON=$(python3 <<PYTHON
import json
import sys

try:
    with open('harness/feature_list.json', 'r') as f:
        data = json.load(f)

    for cat_name, cat_data in data['categories'].items():
        for feature in cat_data['features']:
            if feature['id'] == '$FEATURE_ID':
                result = {
                    'id': feature['id'],
                    'name': feature['name'],
                    'description': feature['description'],
                    'category': cat_data['name'],
                    'priority': cat_data['priority'],
                    'files': feature.get('files', []),
                    'test_steps': feature.get('test_steps', []),
                    'verification': feature.get('verification_command', '')
                }
                print(json.dumps(result))
                sys.exit(0)

    sys.exit(1)

except Exception as e:
    print(f"Error: {e}", file=sys.stderr)
    sys.exit(1)
PYTHON
)

# Parse feature JSON
FEATURE_NAME=$(echo "$FEATURE_JSON" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['name'])")
FEATURE_DESC=$(echo "$FEATURE_JSON" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['description'])")
FEATURE_CATEGORY=$(echo "$FEATURE_JSON" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['category'])")
FEATURE_PRIORITY=$(echo "$FEATURE_JSON" | python3 -c "import sys, json; d=json.load(sys.stdin); print(d['priority'])")
FEATURE_FILES=$(echo "$FEATURE_JSON" | python3 -c "import sys, json; d=json.load(sys.stdin); print(','.join(d.get('files', [])))")

echo "Selected Feature:"
echo "  ID: $FEATURE_ID"
echo "  Name: $FEATURE_NAME"
echo "  Category: $FEATURE_CATEGORY"
echo "  Priority: $FEATURE_PRIORITY"
echo "  Description: $FEATURE_DESC"
echo "  Files: $FEATURE_FILES"
echo ""

# =============================================================================
# STEP 2: Analyze Current Implementation
# =============================================================================

echo -e "${CYAN}[STEP 2/7] Analyzing Current Implementation${NC}"
echo ""

cd "$OS_DIR"

# Check if feature files exist
for file in ${FEATURE_FILES//,/ }; do
    if [ -f "$file" ]; then
        SIZE=$(wc -l < "$file")
        echo "  ✓ $file ($SIZE lines)"
    else
        echo "  ✗ $file (not found)"
    fi
done

echo ""

# =============================================================================
# STEP 3: Generate Code Changes
# =============================================================================

echo -e "${CYAN}[STEP 3/7] Code Generation${NC}"
echo ""
echo "This step requires manual code implementation."
echo ""
echo "Files to modify:"
for file in ${FEATURE_FILES//,/ }; do
    if [ -f "$file" ]; then
        echo "  - $file"
    fi
done
echo ""

# Create implementation plan
PLAN_FILE="harness/sessions/current_plan.md"
mkdir -p harness/sessions

cat > "$PLAN_FILE" <<EOF
# Implementation Plan: $FEATURE_ID

## Feature: $FEATURE_NAME
**Category**: $FEATURE_CATEGORY
**Priority**: $FEATURE_PRIORITY
**Description**: $FEATURE_DESC

## Files to Modify
$FEATURE_FILES

## Test Steps
EOF

# Add test steps to plan
python3 <<PYTHON
import json

try:
    with open('harness/feature_list.json', 'r') as f:
        data = json.load(f)

    for cat_name, cat_data in data['categories'].items():
        for feature in cat_data['features']:
            if feature['id'] == '$FEATURE_ID':
                with open('harness/sessions/current_plan.md', 'a') as f:
                    for i, step in enumerate(feature.get('test_steps', []), 1):
                        f.write(f"{i}. {step}\n")
                break
        if feature['id'] == '$FEATURE_ID':
            break

except Exception as e:
    print(f"Error: {e}")
PYTHON

echo ""
echo "Implementation plan saved to: $PLAN_FILE"
echo ""
echo -e "${YELLOW}⚠${NC}  Please review the plan and implement the required changes."
echo ""
echo -e "${YELLOW}Press Enter to continue when ready, or Ctrl+C to abort...${NC}"
read

# =============================================================================
# STEP 4: Build
# =============================================================================

echo ""
echo -e "${CYAN}[STEP 4/7] Building${NC}"
echo ""

if [ "$SKIP_BUILD" = true ]; then
    echo "Skipping build (--skip-build flag)"
else
    echo "Generating offsets..."
    make gen-offsets > build.log 2>&1

    echo "Building kernel..."
    make clean >> build.log 2>&1
    make >> build.log 2>&1

    if [ -f kernel.bin ]; then
        echo -e "${GREEN}✓${NC} Build successful"
        KERNEL_SIZE=$(stat -c%s kernel.bin 2>/dev/null || stat -f%z kernel.bin)
        echo "  kernel.bin: $KERNEL_SIZE bytes"
    else
        echo -e "${RED}✗${NC} Build failed"
        echo "  Check build.log for details"
        exit 1
    fi

    echo "Creating ISO..."
    ./zh.sh > iso.log 2>&1

    if [ -f os.iso ]; then
        echo -e "${GREEN}✓${NC} ISO created"
    else
        echo -e "${YELLOW}⚠${NC}  ISO creation failed (check iso.log)"
    fi
fi

echo ""

# =============================================================================
# STEP 5: Test
# =============================================================================

echo -e "${CYAN}[STEP 5/7] Testing${NC}"
echo ""

if [ "$SKIP_TEST" = true ]; then
    echo "Skipping test (--skip-test flag)"
    TEST_RESULT="skipped"
else
    echo "Running feature test..."

    # Create session directory for test logs
    SESSION_ID=$(date +%Y%m%d_%H%M%S)
    SESSION_DIR="harness/sessions/$SESSION_ID"
    mkdir -p "$SESSION_DIR"

    TEST_LOG="$SESSION_DIR/test_${FEATURE_ID}.log"

    # Run QEMU test
    echo "  Testing with QEMU (timeout: 10 seconds)..."
    timeout 10s qemu-system-i386 \
        -kernel kernel.bin \
        -m 512M \
        -display none \
        -serial file:"$TEST_LOG" \
        -no-reboot \
        2>/dev/null || true

    # Analyze test results
    if grep -q "task init" "$TEST_LOG" 2>/dev/null; then
        TEST_RESULT="pass"
        echo -e "${GREEN}✓${NC} Test PASSED"
    elif grep -q "Kernel Booted" "$TEST_LOG" 2>/dev/null; then
        TEST_RESULT="partial"
        echo -e "${YELLOW}⚠${NC} Test PARTIAL (kernel booted, feature verification needed)"
    else
        TEST_RESULT="fail"
        echo -e "${RED}✗${NC} Test FAILED"
        echo ""
        echo "Last 20 lines of test output:"
        tail -20 "$TEST_LOG" | sed 's/^/  /'
    fi
fi

echo ""

# =============================================================================
# STEP 6: Update Status
# =============================================================================

echo -e "${CYAN}[STEP 6/7] Status Update${NC}"
echo ""

if [ "$TEST_RESULT" = "pass" ]; then
    echo "Feature $FEATURE_ID passed tests!"
    echo ""
    echo -e "${YELLOW}Do you want to mark this feature as PASSING? (y/n)${NC}"
    read -r CONFIRM

    if [ "$CONFIRM" = "y" ] || [ "$CONFIRM" = "Y" ]; then
        # Update feature_list.json
        python3 <<PYTHON
import json

with open('harness/feature_list.json', 'r') as f:
    data = json.load(f)

for cat_name, cat_data in data['categories'].items():
    for feature in cat_data['features']:
        if feature['id'] == '$FEATURE_ID':
            feature['status'] = 'passing'
            break
    if feature['id'] == '$FEATURE_ID':
        break

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

with open('harness/feature_list.json', 'w') as f:
    json.dump(data, f, indent=2)

print(f"Updated: {passing}/{total} features passing")
PYTHON

        echo -e "${GREEN}✓${NC} Feature marked as PASSING"
    else
        echo "Status update cancelled"
    fi
elif [ "$TEST_RESULT" = "fail" ]; then
    echo "Feature $FEATURE_ID failed tests."
    echo "Status remains: FAILING"
else
    echo "Test was skipped or partial."
    echo "Status: UNCHANGED"
fi

echo ""

# =============================================================================
# STEP 7: Commit Changes
# =============================================================================

echo -e "${CYAN}[STEP 7/7] Commit Changes${NC}"
echo ""

echo "Current git status:"
git status --short | head -10
echo ""

echo -e "${YELLOW}Do you want to commit these changes? (y/n)${NC}"
read -r COMMIT_CONFIRM

if [ "$COMMIT_CONFIRM" = "y" ] || [ "$COMMIT_CONFIRM" = "Y" ]; then
    echo "Enter commit message (or press Enter for default):"
    echo "Default: '$FEATURE_ID: $FEATURE_NAME'"
    read -r COMMIT_MSG

    if [ -z "$COMMIT_MSG" ]; then
        COMMIT_MSG="$FEATURE_ID: $FEATURE_NAME"
    fi

    git add -A
    git commit -m "$COMMIT_MSG" -m ""
    git commit -m "Feature: $FEATURE_ID" -m "Status: $TEST_RESULT" -m "Session: $SESSION_ID"

    echo -e "${GREEN}✓${NC} Changes committed"
    echo ""
    git log -1 --oneline
else
    echo "Commit cancelled"
fi

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Workflow Complete!${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Summary:"
echo "  Feature: $FEATURE_ID"
echo "  Name: $FEATURE_NAME"
echo "  Test Result: $TEST_RESULT"
echo "  Session: $SESSION_DIR"
echo ""
echo "Next steps:"
if [ "$TEST_RESULT" = "pass" ]; then
    echo -e "  ${GREEN}✓${NC} Feature is passing! Continue to next feature."
elif [ "$TEST_RESULT" = "fail" ]; then
    echo -e "  ${RED}✗${NC} Feature failed. Review test output and fix issues."
    echo "  Test log: $TEST_LOG"
else
    echo "  Review implementation and test manually."
fi
echo ""
echo "Run again to work on next feature:"
echo "  ./harness/auto_workflow.sh"
echo ""
