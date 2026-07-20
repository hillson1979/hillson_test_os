# compare_map.py
import re
import sys

def parse_map(filename):
    """
    解析 map 文件，提取各段地址和大小，以及 stack_top
    """
    segments = {}
    stack_top = None

    with open(filename, 'r') as f:
        for line in f:
            # 匹配段地址和大小，例如：
            # .text           0x800000    0x1000
            m = re.match(r'^\s*(\.\w+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)', line)
            if m:
                name = m.group(1)
                addr = int(m.group(2), 16)
                size = int(m.group(3), 16)
                segments[name] = (addr, size)

            # 匹配 stack_top 符号，例如：
            # stack_top       0xC02FF000
            m2 = re.match(r'^\s*stack_top\s+0x([0-9a-fA-F]+)', line)
            if m2:
                stack_top = int(m2.group(1), 16)

    return segments, stack_top

def compare_segments(seg1, seg2):
    keys = set(seg1.keys()).union(seg2.keys())
    print("段比较结果：")
    for k in sorted(keys):
        a = seg1.get(k, (0,0))
        b = seg2.get(k, (0,0))
        if a != b:
            print(f"{k:10} 项目A: 0x{a[0]:08X}+0x{a[1]:X}  项目B: 0x{b[0]:08X}+0x{b[1]:X}")

def main(map1, map2):
    seg1, stack1 = parse_map(map1)
    seg2, stack2 = parse_map(map2)

    compare_segments(seg1, seg2)

    print("\nstack_top 比较：")
    print(f"项目A: 0x{stack1:08X}")
    print(f"项目B: 0x{stack2:08X}")
    if stack1 != stack2:
        print("⚠️ stack_top 不同！")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"用法: python {sys.argv[0]} projectA.map projectB.map")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])