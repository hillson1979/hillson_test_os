#!/bin/bash
# OS Development Harness - Feature Tester
#
# Tests a specific feature and reports results
#
# Usage: ./test_feature.sh <FEATURE_ID> [test_command]

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

FEATURE_ID=${1}
TEST_CMD=${2:-"auto"}

if [ -z "$FEATURE_ID" ]; then
    echo "Usage: ./test_feature.sh <FEATURE_ID> [test_command]"
    echo ""
    echo "Examples:"
    echo "  ./test_feature.sh SI-001 auto"
    echo "  ./test_feature.sh SI-001 'qemu-system-i386 -kernel kernel.bin -serial file:test.log'"
    exit 1
fi

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}Testing Feature: $FEATURE_ID${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Get session directory
SESSION_DIR=$(ls -t harness/sessions/ | head -1)
SESSION_PATH="harness/sessions/$SESSION_DIR"
TEST_LOG="$SESSION_PATH/test_${FEATURE_ID}.log"

# Create test log
mkdir -p "$SESSION_PATH"
echo "# Test Log for $FEATURE_ID" > "$TEST_LOG"
echo "# Started: $(date)" >> "$TEST_LOG"
echo "" >> "$TEST_LOG"

echo -e "${GREEN}[1/4] Feature Information${NC}"

# Get feature info from feature_list.json
if [ -f harness/feature_list.json ]; then
    FEATURE_INFO=$(python3 -c "
import json
import sys

try:
    with open('harness/feature_list.json', 'r') as f:
        data = json.load(f)

    for cat_name, cat_data in data['categories'].items():
        for feature in cat_data['features']:
            if feature['id'] == '$FEATURE_ID':
                print(f\"{feature['name']}\")
                print(f\"{feature['description']}\")
                print(f\"{cat_data['priority']}\")
                sys.exit(0)

    print('NOT_FOUND')
    sys.exit(1)
except:
    print('ERROR')
    sys.exit(1)
" 2>/dev/null || echo "NOT_FOUND")

    if [ "$FEATURE_INFO" = "NOT_FOUND" ]; then
        echo -e "${YELLOW}⚠${NC} Feature $FEATURE_ID not found in feature_list.json"
    elif [ "$FEATURE_INFO" = "ERROR" ]; then
        echo -e "${RED}✗${NC} Error reading feature list"
    else
        FEATURE_NAME=$(echo "$FEATURE_INFO" | sed -n '1p')
        FEATURE_DESC=$(echo "$FEATURE_INFO" | sed -n '2p')
        FEATURE_PRIORITY=$(echo "$FEATURE_INFO" | sed -n '3p')

        echo "  Name: $FEATURE_NAME"
        echo "  Description: $FEATURE_DESC"
        echo "  Priority: $FEATURE_PRIORITY"
    fi
fi

echo ""
echo -e "${GREEN}[2/4] Running Test${NC}"

if [ "$TEST_CMD" = "auto" ]; then
    echo "  Running automated test..."
    echo "  Command: qemu-system-i386 -kernel kernel.bin -m 512M"

    # Run QEMU with timeout
    timeout 10s qemu-system-i386 \
        -kernel kernel.bin \
        -m 512M \
        -display none \
        -serial file:"$TEST_LOG" \
        -no-reboot \
        2>/dev/null || true

    # Check if test passed
    if grep -q "task init" "$TEST_LOG" 2>/dev/null; then
        TEST_RESULT="PASS"
        TEST_REASON="Kernel booted successfully"
    else
        TEST_RESULT="FAIL"
        TEST_REASON="Kernel failed to boot properly"
    fi
else
    echo "  Running custom test..."
    echo "  Command: $TEST_CMD"

    # Run custom test command
    eval "$TEST_CMD" >> "$TEST_LOG" 2>&1
    TEST_EXIT=$?

    if [ $TEST_EXIT -eq 0 ]; then
        TEST_RESULT="PASS"
        TEST_REASON="Test command exited successfully"
    else
        TEST_RESULT="FAIL"
        TEST_REASON="Test command failed with exit code $TEST_EXIT"
    fi
fi

echo ""
echo -e "${GREEN}[3/4] Test Results${NC}"

echo "  Result: $TEST_RESULT"
echo "  Reason: $TEST_REASON"

# Show last few lines of test log
if [ -f "$TEST_LOG" ]; then
    echo ""
    echo "  Last 10 lines of test output:"
    tail -10 "$TEST_LOG" | sed 's/^/    /'
fi

echo ""
echo -e "${GREEN}[4/4] Summary${NC}"

if [ "$TEST_RESULT" = "PASS" ]; then
    echo -e "  ${GREEN}✓${NC} Feature $FEATURE_ID: PASSED"
    echo ""
    echo "  Next steps:"
    echo -e "    Mark as passing: ${YELLOW}./harness/update_progress.sh $FEATURE_ID passing${NC}"
else
    echo -e "  ${RED}✗${NC} Feature $FEATURE_ID: FAILED"
    echo ""
    echo "  Next steps:"
    echo "    1. Review test output: cat $TEST_LOG"
    echo "    2. Fix issues"
    echo "    3. Re-test: ./harness/test_feature.sh $FEATURE_ID"
fi

echo ""
echo -e "${BLUE}======================================${NC}"
echo ""
