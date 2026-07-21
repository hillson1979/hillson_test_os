/**
 * HelloWorld.java — 最简 Java 测试程序
 *
 * 编译:
 *   javac HelloWorld.java
 *
 * 在 HillsonOS 上运行:
 *   jvm.elf HelloWorld
 */
public class HelloWorld {
    public static void main(String[] args) {
        System.out.println("Hello, HillsonOS from Java!");
        System.out.println("JVM is running on bare-metal x86!");

        if (args.length > 0) {
            System.out.println("Arguments:");
            for (int i = 0; i < args.length; i++) {
                System.out.println("  [" + i + "] " + args[i]);
            }
        }
    }
}
