
editor_test.elf:     file format elf32-i386


Disassembly of section .text:

08000000 <_start>:
 8000000:	55                   	push   %ebp
 8000001:	89 e5                	mov    %esp,%ebp
 8000003:	83 ec 18             	sub    $0x18,%esp
 8000006:	b8 e0 57 00 08       	mov    $0x80057e0,%eax
 800000b:	2d e0 57 00 08       	sub    $0x80057e0,%eax
 8000010:	c1 f8 02             	sar    $0x2,%eax
 8000013:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8000016:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 800001d:	eb 10                	jmp    800002f <_start+0x2f>
 800001f:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000022:	8b 04 85 e0 57 00 08 	mov    0x80057e0(,%eax,4),%eax
 8000029:	ff d0                	call   *%eax
 800002b:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 800002f:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000032:	3b 45 f0             	cmp    -0x10(%ebp),%eax
 8000035:	72 e8                	jb     800001f <_start+0x1f>
 8000037:	e8 d8 30 00 00       	call   8003114 <main>
 800003c:	89 45 ec             	mov    %eax,-0x14(%ebp)
 800003f:	f4                   	hlt
 8000040:	eb fd                	jmp    800003f <_start+0x3f>

08000042 <__cxa_atexit>:
 8000042:	55                   	push   %ebp
 8000043:	89 e5                	mov    %esp,%ebp
 8000045:	b8 00 00 00 00       	mov    $0x0,%eax
 800004a:	5d                   	pop    %ebp
 800004b:	c3                   	ret

0800004c <__cxa_pure_virtual>:
 800004c:	55                   	push   %ebp
 800004d:	89 e5                	mov    %esp,%ebp
 800004f:	83 ec 08             	sub    $0x8,%esp
 8000052:	83 ec 0c             	sub    $0xc,%esp
 8000055:	68 a0 4c 00 08       	push   $0x8004ca0
 800005a:	e8 22 3c 00 00       	call   8003c81 <printf>
 800005f:	83 c4 10             	add    $0x10,%esp
 8000062:	f4                   	hlt
 8000063:	eb fd                	jmp    8000062 <__cxa_pure_virtual+0x16>

08000065 <__gxx_personality_v0>:
 8000065:	55                   	push   %ebp
 8000066:	89 e5                	mov    %esp,%ebp
 8000068:	90                   	nop
 8000069:	5d                   	pop    %ebp
 800006a:	c3                   	ret

0800006b <_Unwind_Resume>:
 800006b:	55                   	push   %ebp
 800006c:	89 e5                	mov    %esp,%ebp
 800006e:	90                   	nop
 800006f:	5d                   	pop    %ebp
 8000070:	c3                   	ret

08000071 <__stack_chk_fail>:
 8000071:	55                   	push   %ebp
 8000072:	89 e5                	mov    %esp,%ebp
 8000074:	83 ec 08             	sub    $0x8,%esp
 8000077:	83 ec 0c             	sub    $0xc,%esp
 800007a:	68 cc 4c 00 08       	push   $0x8004ccc
 800007f:	e8 fd 3b 00 00       	call   8003c81 <printf>
 8000084:	83 c4 10             	add    $0x10,%esp
 8000087:	f4                   	hlt
 8000088:	eb fd                	jmp    8000087 <__stack_chk_fail+0x16>

0800008a <_Znwj>:
 800008a:	55                   	push   %ebp
 800008b:	89 e5                	mov    %esp,%ebp
 800008d:	83 ec 18             	sub    $0x18,%esp
 8000090:	8b 15 e0 57 00 08    	mov    0x80057e0,%edx
 8000096:	8b 45 08             	mov    0x8(%ebp),%eax
 8000099:	01 d0                	add    %edx,%eax
 800009b:	ba 20 58 10 08       	mov    $0x8105820,%edx
 80000a0:	39 c2                	cmp    %eax,%edx
 80000a2:	73 16                	jae    80000ba <_Znwj+0x30>
 80000a4:	83 ec 08             	sub    $0x8,%esp
 80000a7:	ff 75 08             	push   0x8(%ebp)
 80000aa:	68 f4 4c 00 08       	push   $0x8004cf4
 80000af:	e8 cd 3b 00 00       	call   8003c81 <printf>
 80000b4:	83 c4 10             	add    $0x10,%esp
 80000b7:	f4                   	hlt
 80000b8:	eb fd                	jmp    80000b7 <_Znwj+0x2d>
 80000ba:	a1 e0 57 00 08       	mov    0x80057e0,%eax
 80000bf:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80000c2:	a1 e0 57 00 08       	mov    0x80057e0,%eax
 80000c7:	8b 55 08             	mov    0x8(%ebp),%edx
 80000ca:	83 c2 0f             	add    $0xf,%edx
 80000cd:	83 e2 f0             	and    $0xfffffff0,%edx
 80000d0:	01 d0                	add    %edx,%eax
 80000d2:	a3 e0 57 00 08       	mov    %eax,0x80057e0
 80000d7:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80000da:	c9                   	leave
 80000db:	c3                   	ret

080000dc <_Znaj>:
 80000dc:	55                   	push   %ebp
 80000dd:	89 e5                	mov    %esp,%ebp
 80000df:	83 ec 08             	sub    $0x8,%esp
 80000e2:	83 ec 0c             	sub    $0xc,%esp
 80000e5:	ff 75 08             	push   0x8(%ebp)
 80000e8:	e8 9d ff ff ff       	call   800008a <_Znwj>
 80000ed:	83 c4 10             	add    $0x10,%esp
 80000f0:	c9                   	leave
 80000f1:	c3                   	ret

080000f2 <_ZdlPv>:
 80000f2:	55                   	push   %ebp
 80000f3:	89 e5                	mov    %esp,%ebp
 80000f5:	90                   	nop
 80000f6:	5d                   	pop    %ebp
 80000f7:	c3                   	ret

080000f8 <_ZdaPv>:
 80000f8:	55                   	push   %ebp
 80000f9:	89 e5                	mov    %esp,%ebp
 80000fb:	90                   	nop
 80000fc:	5d                   	pop    %ebp
 80000fd:	c3                   	ret

080000fe <_ZdlPvj>:
 80000fe:	55                   	push   %ebp
 80000ff:	89 e5                	mov    %esp,%ebp
 8000101:	90                   	nop
 8000102:	5d                   	pop    %ebp
 8000103:	c3                   	ret

08000104 <_ZdaPvj>:
 8000104:	55                   	push   %ebp
 8000105:	89 e5                	mov    %esp,%ebp
 8000107:	90                   	nop
 8000108:	5d                   	pop    %ebp
 8000109:	c3                   	ret

0800010a <__cxa_guard_acquire>:
 800010a:	55                   	push   %ebp
 800010b:	89 e5                	mov    %esp,%ebp
 800010d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000110:	8b 50 04             	mov    0x4(%eax),%edx
 8000113:	8b 00                	mov    (%eax),%eax
 8000115:	89 c1                	mov    %eax,%ecx
 8000117:	89 c8                	mov    %ecx,%eax
 8000119:	09 d0                	or     %edx,%eax
 800011b:	0f 94 c0             	sete   %al
 800011e:	0f b6 c0             	movzbl %al,%eax
 8000121:	5d                   	pop    %ebp
 8000122:	c3                   	ret

08000123 <__cxa_guard_release>:
 8000123:	55                   	push   %ebp
 8000124:	89 e5                	mov    %esp,%ebp
 8000126:	8b 45 08             	mov    0x8(%ebp),%eax
 8000129:	c7 00 01 00 00 00    	movl   $0x1,(%eax)
 800012f:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8000136:	90                   	nop
 8000137:	5d                   	pop    %ebp
 8000138:	c3                   	ret

08000139 <__cxa_guard_abort>:
 8000139:	55                   	push   %ebp
 800013a:	89 e5                	mov    %esp,%ebp
 800013c:	8b 45 08             	mov    0x8(%ebp),%eax
 800013f:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 8000145:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 800014c:	90                   	nop
 800014d:	5d                   	pop    %ebp
 800014e:	c3                   	ret
 800014f:	90                   	nop

08000150 <_ZL13strdup_simplePKc>:
 8000150:	55                   	push   %ebp
 8000151:	89 e5                	mov    %esp,%ebp
 8000153:	83 ec 18             	sub    $0x18,%esp
 8000156:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 800015a:	75 07                	jne    8000163 <_ZL13strdup_simplePKc+0x13>
 800015c:	b8 00 00 00 00       	mov    $0x0,%eax
 8000161:	eb 5e                	jmp    80001c1 <_ZL13strdup_simplePKc+0x71>
 8000163:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 800016a:	eb 04                	jmp    8000170 <_ZL13strdup_simplePKc+0x20>
 800016c:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8000170:	8b 55 08             	mov    0x8(%ebp),%edx
 8000173:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000176:	01 d0                	add    %edx,%eax
 8000178:	0f b6 00             	movzbl (%eax),%eax
 800017b:	84 c0                	test   %al,%al
 800017d:	75 ed                	jne    800016c <_ZL13strdup_simplePKc+0x1c>
 800017f:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000182:	83 c0 01             	add    $0x1,%eax
 8000185:	83 ec 0c             	sub    $0xc,%esp
 8000188:	50                   	push   %eax
 8000189:	e8 4e ff ff ff       	call   80000dc <_Znaj>
 800018e:	83 c4 10             	add    $0x10,%esp
 8000191:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8000194:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 800019b:	eb 19                	jmp    80001b6 <_ZL13strdup_simplePKc+0x66>
 800019d:	8b 55 08             	mov    0x8(%ebp),%edx
 80001a0:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80001a3:	01 d0                	add    %edx,%eax
 80001a5:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 80001a8:	8b 55 f0             	mov    -0x10(%ebp),%edx
 80001ab:	01 ca                	add    %ecx,%edx
 80001ad:	0f b6 00             	movzbl (%eax),%eax
 80001b0:	88 02                	mov    %al,(%edx)
 80001b2:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 80001b6:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80001b9:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 80001bc:	73 df                	jae    800019d <_ZL13strdup_simplePKc+0x4d>
 80001be:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80001c1:	c9                   	leave
 80001c2:	c3                   	ret

080001c3 <_ZL13strcmp_simplePKcS0_>:
 80001c3:	55                   	push   %ebp
 80001c4:	89 e5                	mov    %esp,%ebp
 80001c6:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 80001ca:	75 0d                	jne    80001d9 <_ZL13strcmp_simplePKcS0_+0x16>
 80001cc:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80001d0:	75 07                	jne    80001d9 <_ZL13strcmp_simplePKcS0_+0x16>
 80001d2:	b8 00 00 00 00       	mov    $0x0,%eax
 80001d7:	eb 5c                	jmp    8000235 <_ZL13strcmp_simplePKcS0_+0x72>
 80001d9:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 80001dd:	75 07                	jne    80001e6 <_ZL13strcmp_simplePKcS0_+0x23>
 80001df:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80001e4:	eb 4f                	jmp    8000235 <_ZL13strcmp_simplePKcS0_+0x72>
 80001e6:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80001ea:	75 0f                	jne    80001fb <_ZL13strcmp_simplePKcS0_+0x38>
 80001ec:	b8 01 00 00 00       	mov    $0x1,%eax
 80001f1:	eb 42                	jmp    8000235 <_ZL13strcmp_simplePKcS0_+0x72>
 80001f3:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 80001f7:	83 45 0c 01          	addl   $0x1,0xc(%ebp)
 80001fb:	8b 45 08             	mov    0x8(%ebp),%eax
 80001fe:	0f b6 00             	movzbl (%eax),%eax
 8000201:	84 c0                	test   %al,%al
 8000203:	74 1a                	je     800021f <_ZL13strcmp_simplePKcS0_+0x5c>
 8000205:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000208:	0f b6 00             	movzbl (%eax),%eax
 800020b:	84 c0                	test   %al,%al
 800020d:	74 10                	je     800021f <_ZL13strcmp_simplePKcS0_+0x5c>
 800020f:	8b 45 08             	mov    0x8(%ebp),%eax
 8000212:	0f b6 10             	movzbl (%eax),%edx
 8000215:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000218:	0f b6 00             	movzbl (%eax),%eax
 800021b:	38 c2                	cmp    %al,%dl
 800021d:	74 d4                	je     80001f3 <_ZL13strcmp_simplePKcS0_+0x30>
 800021f:	8b 45 08             	mov    0x8(%ebp),%eax
 8000222:	0f b6 00             	movzbl (%eax),%eax
 8000225:	0f b6 c8             	movzbl %al,%ecx
 8000228:	8b 45 0c             	mov    0xc(%ebp),%eax
 800022b:	0f b6 00             	movzbl (%eax),%eax
 800022e:	0f b6 d0             	movzbl %al,%edx
 8000231:	89 c8                	mov    %ecx,%eax
 8000233:	29 d0                	sub    %edx,%eax
 8000235:	5d                   	pop    %ebp
 8000236:	c3                   	ret
 8000237:	90                   	nop

08000238 <_ZN7QObjectC1EPS_PKc>:
 8000238:	55                   	push   %ebp
 8000239:	89 e5                	mov    %esp,%ebp
 800023b:	83 ec 08             	sub    $0x8,%esp
 800023e:	ba 2c 4d 00 08       	mov    $0x8004d2c,%edx
 8000243:	8b 45 08             	mov    0x8(%ebp),%eax
 8000246:	89 10                	mov    %edx,(%eax)
 8000248:	8b 45 08             	mov    0x8(%ebp),%eax
 800024b:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8000252:	8b 45 08             	mov    0x8(%ebp),%eax
 8000255:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
 800025c:	8b 45 08             	mov    0x8(%ebp),%eax
 800025f:	c7 40 0c 00 00 00 00 	movl   $0x0,0xc(%eax)
 8000266:	8b 45 08             	mov    0x8(%ebp),%eax
 8000269:	c7 40 10 00 00 00 00 	movl   $0x0,0x10(%eax)
 8000270:	8b 45 08             	mov    0x8(%ebp),%eax
 8000273:	c7 40 14 00 00 00 00 	movl   $0x0,0x14(%eax)
 800027a:	8b 45 08             	mov    0x8(%ebp),%eax
 800027d:	c7 40 18 00 00 00 00 	movl   $0x0,0x18(%eax)
 8000284:	83 ec 0c             	sub    $0xc,%esp
 8000287:	ff 75 10             	push   0x10(%ebp)
 800028a:	e8 c1 fe ff ff       	call   8000150 <_ZL13strdup_simplePKc>
 800028f:	83 c4 10             	add    $0x10,%esp
 8000292:	8b 55 08             	mov    0x8(%ebp),%edx
 8000295:	89 42 14             	mov    %eax,0x14(%edx)
 8000298:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 800029c:	74 11                	je     80002af <_ZN7QObjectC1EPS_PKc+0x77>
 800029e:	83 ec 08             	sub    $0x8,%esp
 80002a1:	ff 75 0c             	push   0xc(%ebp)
 80002a4:	ff 75 08             	push   0x8(%ebp)
 80002a7:	e8 72 01 00 00       	call   800041e <_ZN7QObject9setParentEPS_>
 80002ac:	83 c4 10             	add    $0x10,%esp
 80002af:	90                   	nop
 80002b0:	c9                   	leave
 80002b1:	c3                   	ret

080002b2 <_ZN7QObjectD1Ev>:
 80002b2:	55                   	push   %ebp
 80002b3:	89 e5                	mov    %esp,%ebp
 80002b5:	83 ec 28             	sub    $0x28,%esp
 80002b8:	ba 2c 4d 00 08       	mov    $0x8004d2c,%edx
 80002bd:	8b 45 08             	mov    0x8(%ebp),%eax
 80002c0:	89 10                	mov    %edx,(%eax)
 80002c2:	eb 4a                	jmp    800030e <_ZN7QObjectD1Ev+0x5c>
 80002c4:	8b 45 08             	mov    0x8(%ebp),%eax
 80002c7:	8b 40 08             	mov    0x8(%eax),%eax
 80002ca:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 80002cd:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80002d0:	8b 50 0c             	mov    0xc(%eax),%edx
 80002d3:	8b 45 08             	mov    0x8(%ebp),%eax
 80002d6:	89 50 08             	mov    %edx,0x8(%eax)
 80002d9:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80002dc:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 80002e3:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80002e6:	c7 40 0c 00 00 00 00 	movl   $0x0,0xc(%eax)
 80002ed:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80002f0:	c7 40 10 00 00 00 00 	movl   $0x0,0x10(%eax)
 80002f7:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80002fa:	85 c0                	test   %eax,%eax
 80002fc:	74 10                	je     800030e <_ZN7QObjectD1Ev+0x5c>
 80002fe:	8b 10                	mov    (%eax),%edx
 8000300:	83 c2 04             	add    $0x4,%edx
 8000303:	8b 12                	mov    (%edx),%edx
 8000305:	83 ec 0c             	sub    $0xc,%esp
 8000308:	50                   	push   %eax
 8000309:	ff d2                	call   *%edx
 800030b:	83 c4 10             	add    $0x10,%esp
 800030e:	8b 45 08             	mov    0x8(%ebp),%eax
 8000311:	8b 40 08             	mov    0x8(%eax),%eax
 8000314:	85 c0                	test   %eax,%eax
 8000316:	75 ac                	jne    80002c4 <_ZN7QObjectD1Ev+0x12>
 8000318:	8b 45 08             	mov    0x8(%ebp),%eax
 800031b:	8b 40 04             	mov    0x4(%eax),%eax
 800031e:	85 c0                	test   %eax,%eax
 8000320:	74 43                	je     8000365 <_ZN7QObjectD1Ev+0xb3>
 8000322:	8b 45 08             	mov    0x8(%ebp),%eax
 8000325:	8b 40 10             	mov    0x10(%eax),%eax
 8000328:	85 c0                	test   %eax,%eax
 800032a:	74 11                	je     800033d <_ZN7QObjectD1Ev+0x8b>
 800032c:	8b 45 08             	mov    0x8(%ebp),%eax
 800032f:	8b 40 10             	mov    0x10(%eax),%eax
 8000332:	8b 55 08             	mov    0x8(%ebp),%edx
 8000335:	8b 52 0c             	mov    0xc(%edx),%edx
 8000338:	89 50 0c             	mov    %edx,0xc(%eax)
 800033b:	eb 0f                	jmp    800034c <_ZN7QObjectD1Ev+0x9a>
 800033d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000340:	8b 40 04             	mov    0x4(%eax),%eax
 8000343:	8b 55 08             	mov    0x8(%ebp),%edx
 8000346:	8b 52 0c             	mov    0xc(%edx),%edx
 8000349:	89 50 08             	mov    %edx,0x8(%eax)
 800034c:	8b 45 08             	mov    0x8(%ebp),%eax
 800034f:	8b 40 0c             	mov    0xc(%eax),%eax
 8000352:	85 c0                	test   %eax,%eax
 8000354:	74 0f                	je     8000365 <_ZN7QObjectD1Ev+0xb3>
 8000356:	8b 45 08             	mov    0x8(%ebp),%eax
 8000359:	8b 40 0c             	mov    0xc(%eax),%eax
 800035c:	8b 55 08             	mov    0x8(%ebp),%edx
 800035f:	8b 52 10             	mov    0x10(%edx),%edx
 8000362:	89 50 10             	mov    %edx,0x10(%eax)
 8000365:	8b 45 08             	mov    0x8(%ebp),%eax
 8000368:	8b 40 18             	mov    0x18(%eax),%eax
 800036b:	89 45 f4             	mov    %eax,-0xc(%ebp)
 800036e:	eb 59                	jmp    80003c9 <_ZN7QObjectD1Ev+0x117>
 8000370:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000373:	8b 40 08             	mov    0x8(%eax),%eax
 8000376:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8000379:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800037c:	8b 40 04             	mov    0x4(%eax),%eax
 800037f:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8000382:	eb 24                	jmp    80003a8 <_ZN7QObjectD1Ev+0xf6>
 8000384:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000387:	8b 40 0c             	mov    0xc(%eax),%eax
 800038a:	89 45 e8             	mov    %eax,-0x18(%ebp)
 800038d:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000390:	85 c0                	test   %eax,%eax
 8000392:	74 0e                	je     80003a2 <_ZN7QObjectD1Ev+0xf0>
 8000394:	83 ec 08             	sub    $0x8,%esp
 8000397:	6a 10                	push   $0x10
 8000399:	50                   	push   %eax
 800039a:	e8 5f fd ff ff       	call   80000fe <_ZdlPvj>
 800039f:	83 c4 10             	add    $0x10,%esp
 80003a2:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80003a5:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80003a8:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 80003ac:	75 d6                	jne    8000384 <_ZN7QObjectD1Ev+0xd2>
 80003ae:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80003b1:	85 c0                	test   %eax,%eax
 80003b3:	74 0e                	je     80003c3 <_ZN7QObjectD1Ev+0x111>
 80003b5:	83 ec 08             	sub    $0x8,%esp
 80003b8:	6a 0c                	push   $0xc
 80003ba:	50                   	push   %eax
 80003bb:	e8 3e fd ff ff       	call   80000fe <_ZdlPvj>
 80003c0:	83 c4 10             	add    $0x10,%esp
 80003c3:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80003c6:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80003c9:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80003cd:	75 a1                	jne    8000370 <_ZN7QObjectD1Ev+0xbe>
 80003cf:	8b 45 08             	mov    0x8(%ebp),%eax
 80003d2:	8b 40 14             	mov    0x14(%eax),%eax
 80003d5:	85 c0                	test   %eax,%eax
 80003d7:	74 1c                	je     80003f5 <_ZN7QObjectD1Ev+0x143>
 80003d9:	8b 45 08             	mov    0x8(%ebp),%eax
 80003dc:	8b 40 14             	mov    0x14(%eax),%eax
 80003df:	85 c0                	test   %eax,%eax
 80003e1:	74 12                	je     80003f5 <_ZN7QObjectD1Ev+0x143>
 80003e3:	8b 45 08             	mov    0x8(%ebp),%eax
 80003e6:	8b 40 14             	mov    0x14(%eax),%eax
 80003e9:	83 ec 0c             	sub    $0xc,%esp
 80003ec:	50                   	push   %eax
 80003ed:	e8 06 fd ff ff       	call   80000f8 <_ZdaPv>
 80003f2:	83 c4 10             	add    $0x10,%esp
 80003f5:	90                   	nop
 80003f6:	c9                   	leave
 80003f7:	c3                   	ret

080003f8 <_ZN7QObjectD0Ev>:
 80003f8:	55                   	push   %ebp
 80003f9:	89 e5                	mov    %esp,%ebp
 80003fb:	83 ec 08             	sub    $0x8,%esp
 80003fe:	83 ec 0c             	sub    $0xc,%esp
 8000401:	ff 75 08             	push   0x8(%ebp)
 8000404:	e8 a9 fe ff ff       	call   80002b2 <_ZN7QObjectD1Ev>
 8000409:	83 c4 10             	add    $0x10,%esp
 800040c:	83 ec 08             	sub    $0x8,%esp
 800040f:	6a 1c                	push   $0x1c
 8000411:	ff 75 08             	push   0x8(%ebp)
 8000414:	e8 e5 fc ff ff       	call   80000fe <_ZdlPvj>
 8000419:	83 c4 10             	add    $0x10,%esp
 800041c:	c9                   	leave
 800041d:	c3                   	ret

0800041e <_ZN7QObject9setParentEPS_>:
 800041e:	55                   	push   %ebp
 800041f:	89 e5                	mov    %esp,%ebp
 8000421:	8b 45 08             	mov    0x8(%ebp),%eax
 8000424:	8b 40 04             	mov    0x4(%eax),%eax
 8000427:	85 c0                	test   %eax,%eax
 8000429:	74 43                	je     800046e <_ZN7QObject9setParentEPS_+0x50>
 800042b:	8b 45 08             	mov    0x8(%ebp),%eax
 800042e:	8b 40 10             	mov    0x10(%eax),%eax
 8000431:	85 c0                	test   %eax,%eax
 8000433:	74 11                	je     8000446 <_ZN7QObject9setParentEPS_+0x28>
 8000435:	8b 45 08             	mov    0x8(%ebp),%eax
 8000438:	8b 40 10             	mov    0x10(%eax),%eax
 800043b:	8b 55 08             	mov    0x8(%ebp),%edx
 800043e:	8b 52 0c             	mov    0xc(%edx),%edx
 8000441:	89 50 0c             	mov    %edx,0xc(%eax)
 8000444:	eb 0f                	jmp    8000455 <_ZN7QObject9setParentEPS_+0x37>
 8000446:	8b 45 08             	mov    0x8(%ebp),%eax
 8000449:	8b 40 04             	mov    0x4(%eax),%eax
 800044c:	8b 55 08             	mov    0x8(%ebp),%edx
 800044f:	8b 52 0c             	mov    0xc(%edx),%edx
 8000452:	89 50 08             	mov    %edx,0x8(%eax)
 8000455:	8b 45 08             	mov    0x8(%ebp),%eax
 8000458:	8b 40 0c             	mov    0xc(%eax),%eax
 800045b:	85 c0                	test   %eax,%eax
 800045d:	74 0f                	je     800046e <_ZN7QObject9setParentEPS_+0x50>
 800045f:	8b 45 08             	mov    0x8(%ebp),%eax
 8000462:	8b 40 0c             	mov    0xc(%eax),%eax
 8000465:	8b 55 08             	mov    0x8(%ebp),%edx
 8000468:	8b 52 10             	mov    0x10(%edx),%edx
 800046b:	89 50 10             	mov    %edx,0x10(%eax)
 800046e:	8b 45 08             	mov    0x8(%ebp),%eax
 8000471:	8b 55 0c             	mov    0xc(%ebp),%edx
 8000474:	89 50 04             	mov    %edx,0x4(%eax)
 8000477:	8b 45 08             	mov    0x8(%ebp),%eax
 800047a:	c7 40 10 00 00 00 00 	movl   $0x0,0x10(%eax)
 8000481:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8000485:	74 2d                	je     80004b4 <_ZN7QObject9setParentEPS_+0x96>
 8000487:	8b 45 0c             	mov    0xc(%ebp),%eax
 800048a:	8b 50 08             	mov    0x8(%eax),%edx
 800048d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000490:	89 50 0c             	mov    %edx,0xc(%eax)
 8000493:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000496:	8b 40 08             	mov    0x8(%eax),%eax
 8000499:	85 c0                	test   %eax,%eax
 800049b:	74 0c                	je     80004a9 <_ZN7QObject9setParentEPS_+0x8b>
 800049d:	8b 45 0c             	mov    0xc(%ebp),%eax
 80004a0:	8b 40 08             	mov    0x8(%eax),%eax
 80004a3:	8b 55 08             	mov    0x8(%ebp),%edx
 80004a6:	89 50 10             	mov    %edx,0x10(%eax)
 80004a9:	8b 45 0c             	mov    0xc(%ebp),%eax
 80004ac:	8b 55 08             	mov    0x8(%ebp),%edx
 80004af:	89 50 08             	mov    %edx,0x8(%eax)
 80004b2:	eb 0a                	jmp    80004be <_ZN7QObject9setParentEPS_+0xa0>
 80004b4:	8b 45 08             	mov    0x8(%ebp),%eax
 80004b7:	c7 40 0c 00 00 00 00 	movl   $0x0,0xc(%eax)
 80004be:	90                   	nop
 80004bf:	5d                   	pop    %ebp
 80004c0:	c3                   	ret
 80004c1:	90                   	nop

080004c2 <_ZN7QObject13setObjectNameEPKc>:
 80004c2:	55                   	push   %ebp
 80004c3:	89 e5                	mov    %esp,%ebp
 80004c5:	83 ec 08             	sub    $0x8,%esp
 80004c8:	8b 45 08             	mov    0x8(%ebp),%eax
 80004cb:	8b 40 14             	mov    0x14(%eax),%eax
 80004ce:	85 c0                	test   %eax,%eax
 80004d0:	74 1c                	je     80004ee <_ZN7QObject13setObjectNameEPKc+0x2c>
 80004d2:	8b 45 08             	mov    0x8(%ebp),%eax
 80004d5:	8b 40 14             	mov    0x14(%eax),%eax
 80004d8:	85 c0                	test   %eax,%eax
 80004da:	74 12                	je     80004ee <_ZN7QObject13setObjectNameEPKc+0x2c>
 80004dc:	8b 45 08             	mov    0x8(%ebp),%eax
 80004df:	8b 40 14             	mov    0x14(%eax),%eax
 80004e2:	83 ec 0c             	sub    $0xc,%esp
 80004e5:	50                   	push   %eax
 80004e6:	e8 0d fc ff ff       	call   80000f8 <_ZdaPv>
 80004eb:	83 c4 10             	add    $0x10,%esp
 80004ee:	83 ec 0c             	sub    $0xc,%esp
 80004f1:	ff 75 0c             	push   0xc(%ebp)
 80004f4:	e8 57 fc ff ff       	call   8000150 <_ZL13strdup_simplePKc>
 80004f9:	83 c4 10             	add    $0x10,%esp
 80004fc:	8b 55 08             	mov    0x8(%ebp),%edx
 80004ff:	89 42 14             	mov    %eax,0x14(%edx)
 8000502:	90                   	nop
 8000503:	c9                   	leave
 8000504:	c3                   	ret
 8000505:	90                   	nop

08000506 <_ZN7QObject18findOrCreateSignalEPKc>:
 8000506:	55                   	push   %ebp
 8000507:	89 e5                	mov    %esp,%ebp
 8000509:	83 ec 18             	sub    $0x18,%esp
 800050c:	8b 45 08             	mov    0x8(%ebp),%eax
 800050f:	8b 40 18             	mov    0x18(%eax),%eax
 8000512:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000515:	eb 28                	jmp    800053f <_ZN7QObject18findOrCreateSignalEPKc+0x39>
 8000517:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800051a:	8b 00                	mov    (%eax),%eax
 800051c:	ff 75 0c             	push   0xc(%ebp)
 800051f:	50                   	push   %eax
 8000520:	e8 9e fc ff ff       	call   80001c3 <_ZL13strcmp_simplePKcS0_>
 8000525:	83 c4 08             	add    $0x8,%esp
 8000528:	85 c0                	test   %eax,%eax
 800052a:	0f 94 c0             	sete   %al
 800052d:	84 c0                	test   %al,%al
 800052f:	74 05                	je     8000536 <_ZN7QObject18findOrCreateSignalEPKc+0x30>
 8000531:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000534:	eb 49                	jmp    800057f <_ZN7QObject18findOrCreateSignalEPKc+0x79>
 8000536:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000539:	8b 40 08             	mov    0x8(%eax),%eax
 800053c:	89 45 f4             	mov    %eax,-0xc(%ebp)
 800053f:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 8000543:	75 d2                	jne    8000517 <_ZN7QObject18findOrCreateSignalEPKc+0x11>
 8000545:	83 ec 0c             	sub    $0xc,%esp
 8000548:	6a 0c                	push   $0xc
 800054a:	e8 3b fb ff ff       	call   800008a <_Znwj>
 800054f:	83 c4 10             	add    $0x10,%esp
 8000552:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000555:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000558:	8b 55 0c             	mov    0xc(%ebp),%edx
 800055b:	89 10                	mov    %edx,(%eax)
 800055d:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000560:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8000567:	8b 45 08             	mov    0x8(%ebp),%eax
 800056a:	8b 50 18             	mov    0x18(%eax),%edx
 800056d:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000570:	89 50 08             	mov    %edx,0x8(%eax)
 8000573:	8b 45 08             	mov    0x8(%ebp),%eax
 8000576:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8000579:	89 50 18             	mov    %edx,0x18(%eax)
 800057c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800057f:	c9                   	leave
 8000580:	c3                   	ret
 8000581:	90                   	nop

08000582 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_>:
 8000582:	55                   	push   %ebp
 8000583:	89 e5                	mov    %esp,%ebp
 8000585:	83 ec 18             	sub    $0x18,%esp
 8000588:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 800058c:	74 12                	je     80005a0 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x1e>
 800058e:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8000592:	74 0c                	je     80005a0 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x1e>
 8000594:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8000598:	74 06                	je     80005a0 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x1e>
 800059a:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
 800059e:	75 0a                	jne    80005aa <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x28>
 80005a0:	b8 00 00 00 00       	mov    $0x0,%eax
 80005a5:	e9 99 00 00 00       	jmp    8000643 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0xc1>
 80005aa:	83 ec 08             	sub    $0x8,%esp
 80005ad:	ff 75 0c             	push   0xc(%ebp)
 80005b0:	ff 75 08             	push   0x8(%ebp)
 80005b3:	e8 4e ff ff ff       	call   8000506 <_ZN7QObject18findOrCreateSignalEPKc>
 80005b8:	83 c4 10             	add    $0x10,%esp
 80005bb:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80005be:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80005c1:	8b 40 04             	mov    0x4(%eax),%eax
 80005c4:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80005c7:	eb 30                	jmp    80005f9 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x77>
 80005c9:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80005cc:	8b 00                	mov    (%eax),%eax
 80005ce:	39 45 10             	cmp    %eax,0x10(%ebp)
 80005d1:	75 1d                	jne    80005f0 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x6e>
 80005d3:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80005d6:	8b 40 04             	mov    0x4(%eax),%eax
 80005d9:	39 45 14             	cmp    %eax,0x14(%ebp)
 80005dc:	75 12                	jne    80005f0 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x6e>
 80005de:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80005e1:	8b 40 08             	mov    0x8(%eax),%eax
 80005e4:	39 45 18             	cmp    %eax,0x18(%ebp)
 80005e7:	75 07                	jne    80005f0 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x6e>
 80005e9:	b8 00 00 00 00       	mov    $0x0,%eax
 80005ee:	eb 53                	jmp    8000643 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0xc1>
 80005f0:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80005f3:	8b 40 0c             	mov    0xc(%eax),%eax
 80005f6:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80005f9:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80005fd:	75 ca                	jne    80005c9 <_ZN7QObject7connectEPS_PKcS0_PFvS0_PvES3_+0x47>
 80005ff:	83 ec 0c             	sub    $0xc,%esp
 8000602:	6a 10                	push   $0x10
 8000604:	e8 81 fa ff ff       	call   800008a <_Znwj>
 8000609:	83 c4 10             	add    $0x10,%esp
 800060c:	89 45 ec             	mov    %eax,-0x14(%ebp)
 800060f:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000612:	8b 55 10             	mov    0x10(%ebp),%edx
 8000615:	89 10                	mov    %edx,(%eax)
 8000617:	8b 45 ec             	mov    -0x14(%ebp),%eax
 800061a:	8b 55 14             	mov    0x14(%ebp),%edx
 800061d:	89 50 04             	mov    %edx,0x4(%eax)
 8000620:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000623:	8b 55 18             	mov    0x18(%ebp),%edx
 8000626:	89 50 08             	mov    %edx,0x8(%eax)
 8000629:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800062c:	8b 50 04             	mov    0x4(%eax),%edx
 800062f:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000632:	89 50 0c             	mov    %edx,0xc(%eax)
 8000635:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000638:	8b 55 ec             	mov    -0x14(%ebp),%edx
 800063b:	89 50 04             	mov    %edx,0x4(%eax)
 800063e:	b8 01 00 00 00       	mov    $0x1,%eax
 8000643:	c9                   	leave
 8000644:	c3                   	ret
 8000645:	90                   	nop

08000646 <_ZN7QObject10emitSignalEPKcPv>:
 8000646:	55                   	push   %ebp
 8000647:	89 e5                	mov    %esp,%ebp
 8000649:	83 ec 18             	sub    $0x18,%esp
 800064c:	8b 45 08             	mov    0x8(%ebp),%eax
 800064f:	8b 40 18             	mov    0x18(%eax),%eax
 8000652:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000655:	eb 6b                	jmp    80006c2 <_ZN7QObject10emitSignalEPKcPv+0x7c>
 8000657:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800065a:	8b 00                	mov    (%eax),%eax
 800065c:	ff 75 0c             	push   0xc(%ebp)
 800065f:	50                   	push   %eax
 8000660:	e8 5e fb ff ff       	call   80001c3 <_ZL13strcmp_simplePKcS0_>
 8000665:	83 c4 08             	add    $0x8,%esp
 8000668:	85 c0                	test   %eax,%eax
 800066a:	0f 94 c0             	sete   %al
 800066d:	84 c0                	test   %al,%al
 800066f:	74 48                	je     80006b9 <_ZN7QObject10emitSignalEPKcPv+0x73>
 8000671:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000674:	8b 40 04             	mov    0x4(%eax),%eax
 8000677:	89 45 f0             	mov    %eax,-0x10(%ebp)
 800067a:	eb 35                	jmp    80006b1 <_ZN7QObject10emitSignalEPKcPv+0x6b>
 800067c:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800067f:	8b 40 08             	mov    0x8(%eax),%eax
 8000682:	85 c0                	test   %eax,%eax
 8000684:	74 08                	je     800068e <_ZN7QObject10emitSignalEPKcPv+0x48>
 8000686:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000689:	8b 40 08             	mov    0x8(%eax),%eax
 800068c:	eb 03                	jmp    8000691 <_ZN7QObject10emitSignalEPKcPv+0x4b>
 800068e:	8b 45 10             	mov    0x10(%ebp),%eax
 8000691:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8000694:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000697:	8b 40 04             	mov    0x4(%eax),%eax
 800069a:	83 ec 08             	sub    $0x8,%esp
 800069d:	ff 75 ec             	push   -0x14(%ebp)
 80006a0:	ff 75 08             	push   0x8(%ebp)
 80006a3:	ff d0                	call   *%eax
 80006a5:	83 c4 10             	add    $0x10,%esp
 80006a8:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80006ab:	8b 40 0c             	mov    0xc(%eax),%eax
 80006ae:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80006b1:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 80006b5:	75 c5                	jne    800067c <_ZN7QObject10emitSignalEPKcPv+0x36>
 80006b7:	eb 0f                	jmp    80006c8 <_ZN7QObject10emitSignalEPKcPv+0x82>
 80006b9:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80006bc:	8b 40 08             	mov    0x8(%eax),%eax
 80006bf:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80006c2:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80006c6:	75 8f                	jne    8000657 <_ZN7QObject10emitSignalEPKcPv+0x11>
 80006c8:	c9                   	leave
 80006c9:	c3                   	ret

080006ca <_ZNK7QObject15connectionCountEv>:
 80006ca:	55                   	push   %ebp
 80006cb:	89 e5                	mov    %esp,%ebp
 80006cd:	83 ec 10             	sub    $0x10,%esp
 80006d0:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 80006d7:	8b 45 08             	mov    0x8(%ebp),%eax
 80006da:	8b 40 18             	mov    0x18(%eax),%eax
 80006dd:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80006e0:	eb 27                	jmp    8000709 <_ZNK7QObject15connectionCountEv+0x3f>
 80006e2:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80006e5:	8b 40 04             	mov    0x4(%eax),%eax
 80006e8:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80006eb:	eb 0d                	jmp    80006fa <_ZNK7QObject15connectionCountEv+0x30>
 80006ed:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 80006f1:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80006f4:	8b 40 0c             	mov    0xc(%eax),%eax
 80006f7:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80006fa:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80006fe:	75 ed                	jne    80006ed <_ZNK7QObject15connectionCountEv+0x23>
 8000700:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8000703:	8b 40 08             	mov    0x8(%eax),%eax
 8000706:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8000709:	83 7d f8 00          	cmpl   $0x0,-0x8(%ebp)
 800070d:	75 d3                	jne    80006e2 <_ZNK7QObject15connectionCountEv+0x18>
 800070f:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8000712:	c9                   	leave
 8000713:	c3                   	ret

08000714 <_ZNK7QObject9classNameEv>:
 8000714:	55                   	push   %ebp
 8000715:	89 e5                	mov    %esp,%ebp
 8000717:	b8 1a 4d 00 08       	mov    $0x8004d1a,%eax
 800071c:	5d                   	pop    %ebp
 800071d:	c3                   	ret

0800071e <_ZN8QPainterC1EPjiii>:
 800071e:	55                   	push   %ebp
 800071f:	89 e5                	mov    %esp,%ebp
 8000721:	8b 45 08             	mov    0x8(%ebp),%eax
 8000724:	8b 55 0c             	mov    0xc(%ebp),%edx
 8000727:	89 10                	mov    %edx,(%eax)
 8000729:	8b 45 08             	mov    0x8(%ebp),%eax
 800072c:	8b 55 10             	mov    0x10(%ebp),%edx
 800072f:	89 50 04             	mov    %edx,0x4(%eax)
 8000732:	8b 45 08             	mov    0x8(%ebp),%eax
 8000735:	8b 55 14             	mov    0x14(%ebp),%edx
 8000738:	89 50 08             	mov    %edx,0x8(%eax)
 800073b:	8b 45 08             	mov    0x8(%ebp),%eax
 800073e:	8b 55 18             	mov    0x18(%ebp),%edx
 8000741:	89 50 0c             	mov    %edx,0xc(%eax)
 8000744:	8b 45 18             	mov    0x18(%ebp),%eax
 8000747:	8d 50 03             	lea    0x3(%eax),%edx
 800074a:	85 c0                	test   %eax,%eax
 800074c:	0f 48 c2             	cmovs  %edx,%eax
 800074f:	c1 f8 02             	sar    $0x2,%eax
 8000752:	89 c2                	mov    %eax,%edx
 8000754:	8b 45 08             	mov    0x8(%ebp),%eax
 8000757:	89 50 10             	mov    %edx,0x10(%eax)
 800075a:	8b 45 08             	mov    0x8(%ebp),%eax
 800075d:	c7 40 14 00 00 00 00 	movl   $0x0,0x14(%eax)
 8000764:	8b 45 08             	mov    0x8(%ebp),%eax
 8000767:	c6 40 18 00          	movb   $0x0,0x18(%eax)
 800076b:	8b 45 08             	mov    0x8(%ebp),%eax
 800076e:	c7 40 1c 00 00 00 00 	movl   $0x0,0x1c(%eax)
 8000775:	8b 45 08             	mov    0x8(%ebp),%eax
 8000778:	c7 40 20 00 00 00 00 	movl   $0x0,0x20(%eax)
 800077f:	8b 45 08             	mov    0x8(%ebp),%eax
 8000782:	c7 40 24 00 00 00 00 	movl   $0x0,0x24(%eax)
 8000789:	8b 45 08             	mov    0x8(%ebp),%eax
 800078c:	c7 40 28 00 00 00 00 	movl   $0x0,0x28(%eax)
 8000793:	90                   	nop
 8000794:	5d                   	pop    %ebp
 8000795:	c3                   	ret

08000796 <_ZN8QPainter8setColorEj>:
 8000796:	55                   	push   %ebp
 8000797:	89 e5                	mov    %esp,%ebp
 8000799:	8b 45 08             	mov    0x8(%ebp),%eax
 800079c:	8b 55 0c             	mov    0xc(%ebp),%edx
 800079f:	89 50 14             	mov    %edx,0x14(%eax)
 80007a2:	90                   	nop
 80007a3:	5d                   	pop    %ebp
 80007a4:	c3                   	ret
 80007a5:	90                   	nop

080007a6 <_ZN8QPainter9drawPixelEii>:
 80007a6:	55                   	push   %ebp
 80007a7:	89 e5                	mov    %esp,%ebp
 80007a9:	83 ec 08             	sub    $0x8,%esp
 80007ac:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80007b0:	78 59                	js     800080b <_ZN8QPainter9drawPixelEii+0x65>
 80007b2:	8b 45 08             	mov    0x8(%ebp),%eax
 80007b5:	8b 40 04             	mov    0x4(%eax),%eax
 80007b8:	39 45 0c             	cmp    %eax,0xc(%ebp)
 80007bb:	7d 4e                	jge    800080b <_ZN8QPainter9drawPixelEii+0x65>
 80007bd:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 80007c1:	78 48                	js     800080b <_ZN8QPainter9drawPixelEii+0x65>
 80007c3:	8b 45 08             	mov    0x8(%ebp),%eax
 80007c6:	8b 40 08             	mov    0x8(%eax),%eax
 80007c9:	39 45 10             	cmp    %eax,0x10(%ebp)
 80007cc:	7d 3d                	jge    800080b <_ZN8QPainter9drawPixelEii+0x65>
 80007ce:	83 ec 04             	sub    $0x4,%esp
 80007d1:	ff 75 10             	push   0x10(%ebp)
 80007d4:	ff 75 0c             	push   0xc(%ebp)
 80007d7:	ff 75 08             	push   0x8(%ebp)
 80007da:	e8 69 04 00 00       	call   8000c48 <_ZNK8QPainter9isClippedEii>
 80007df:	83 c4 10             	add    $0x10,%esp
 80007e2:	84 c0                	test   %al,%al
 80007e4:	75 28                	jne    800080e <_ZN8QPainter9drawPixelEii+0x68>
 80007e6:	8b 45 08             	mov    0x8(%ebp),%eax
 80007e9:	8b 10                	mov    (%eax),%edx
 80007eb:	8b 45 08             	mov    0x8(%ebp),%eax
 80007ee:	8b 40 10             	mov    0x10(%eax),%eax
 80007f1:	0f af 45 10          	imul   0x10(%ebp),%eax
 80007f5:	89 c1                	mov    %eax,%ecx
 80007f7:	8b 45 0c             	mov    0xc(%ebp),%eax
 80007fa:	01 c8                	add    %ecx,%eax
 80007fc:	c1 e0 02             	shl    $0x2,%eax
 80007ff:	01 c2                	add    %eax,%edx
 8000801:	8b 45 08             	mov    0x8(%ebp),%eax
 8000804:	8b 40 14             	mov    0x14(%eax),%eax
 8000807:	89 02                	mov    %eax,(%edx)
 8000809:	eb 04                	jmp    800080f <_ZN8QPainter9drawPixelEii+0x69>
 800080b:	90                   	nop
 800080c:	eb 01                	jmp    800080f <_ZN8QPainter9drawPixelEii+0x69>
 800080e:	90                   	nop
 800080f:	c9                   	leave
 8000810:	c3                   	ret
 8000811:	90                   	nop

08000812 <_ZN8QPainter8fillRectEiiii>:
 8000812:	55                   	push   %ebp
 8000813:	89 e5                	mov    %esp,%ebp
 8000815:	83 ec 18             	sub    $0x18,%esp
 8000818:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 800081c:	79 0d                	jns    800082b <_ZN8QPainter8fillRectEiiii+0x19>
 800081e:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000821:	01 45 14             	add    %eax,0x14(%ebp)
 8000824:	c7 45 0c 00 00 00 00 	movl   $0x0,0xc(%ebp)
 800082b:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 800082f:	79 0d                	jns    800083e <_ZN8QPainter8fillRectEiiii+0x2c>
 8000831:	8b 45 10             	mov    0x10(%ebp),%eax
 8000834:	01 45 18             	add    %eax,0x18(%ebp)
 8000837:	c7 45 10 00 00 00 00 	movl   $0x0,0x10(%ebp)
 800083e:	8b 55 0c             	mov    0xc(%ebp),%edx
 8000841:	8b 45 14             	mov    0x14(%ebp),%eax
 8000844:	01 c2                	add    %eax,%edx
 8000846:	8b 45 08             	mov    0x8(%ebp),%eax
 8000849:	8b 40 04             	mov    0x4(%eax),%eax
 800084c:	39 c2                	cmp    %eax,%edx
 800084e:	7e 0c                	jle    800085c <_ZN8QPainter8fillRectEiiii+0x4a>
 8000850:	8b 45 08             	mov    0x8(%ebp),%eax
 8000853:	8b 40 04             	mov    0x4(%eax),%eax
 8000856:	2b 45 0c             	sub    0xc(%ebp),%eax
 8000859:	89 45 14             	mov    %eax,0x14(%ebp)
 800085c:	8b 55 10             	mov    0x10(%ebp),%edx
 800085f:	8b 45 18             	mov    0x18(%ebp),%eax
 8000862:	01 c2                	add    %eax,%edx
 8000864:	8b 45 08             	mov    0x8(%ebp),%eax
 8000867:	8b 40 08             	mov    0x8(%eax),%eax
 800086a:	39 c2                	cmp    %eax,%edx
 800086c:	7e 0c                	jle    800087a <_ZN8QPainter8fillRectEiiii+0x68>
 800086e:	8b 45 08             	mov    0x8(%ebp),%eax
 8000871:	8b 40 08             	mov    0x8(%eax),%eax
 8000874:	2b 45 10             	sub    0x10(%ebp),%eax
 8000877:	89 45 18             	mov    %eax,0x18(%ebp)
 800087a:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
 800087e:	0f 8e 90 00 00 00    	jle    8000914 <_ZN8QPainter8fillRectEiiii+0x102>
 8000884:	83 7d 18 00          	cmpl   $0x0,0x18(%ebp)
 8000888:	0f 8e 86 00 00 00    	jle    8000914 <_ZN8QPainter8fillRectEiiii+0x102>
 800088e:	8b 45 10             	mov    0x10(%ebp),%eax
 8000891:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000894:	eb 6f                	jmp    8000905 <_ZN8QPainter8fillRectEiiii+0xf3>
 8000896:	8b 45 08             	mov    0x8(%ebp),%eax
 8000899:	0f b6 40 18          	movzbl 0x18(%eax),%eax
 800089d:	84 c0                	test   %al,%al
 800089f:	74 09                	je     80008aa <_ZN8QPainter8fillRectEiiii+0x98>
 80008a1:	8b 45 08             	mov    0x8(%ebp),%eax
 80008a4:	8b 40 20             	mov    0x20(%eax),%eax
 80008a7:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 80008aa:	8b 45 0c             	mov    0xc(%ebp),%eax
 80008ad:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80008b0:	eb 42                	jmp    80008f4 <_ZN8QPainter8fillRectEiiii+0xe2>
 80008b2:	83 ec 04             	sub    $0x4,%esp
 80008b5:	ff 75 f4             	push   -0xc(%ebp)
 80008b8:	ff 75 f0             	push   -0x10(%ebp)
 80008bb:	ff 75 08             	push   0x8(%ebp)
 80008be:	e8 85 03 00 00       	call   8000c48 <_ZNK8QPainter9isClippedEii>
 80008c3:	83 c4 10             	add    $0x10,%esp
 80008c6:	83 f0 01             	xor    $0x1,%eax
 80008c9:	84 c0                	test   %al,%al
 80008cb:	74 23                	je     80008f0 <_ZN8QPainter8fillRectEiiii+0xde>
 80008cd:	8b 45 08             	mov    0x8(%ebp),%eax
 80008d0:	8b 10                	mov    (%eax),%edx
 80008d2:	8b 45 08             	mov    0x8(%ebp),%eax
 80008d5:	8b 40 10             	mov    0x10(%eax),%eax
 80008d8:	0f af 45 f4          	imul   -0xc(%ebp),%eax
 80008dc:	89 c1                	mov    %eax,%ecx
 80008de:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80008e1:	01 c8                	add    %ecx,%eax
 80008e3:	c1 e0 02             	shl    $0x2,%eax
 80008e6:	01 c2                	add    %eax,%edx
 80008e8:	8b 45 08             	mov    0x8(%ebp),%eax
 80008eb:	8b 40 14             	mov    0x14(%eax),%eax
 80008ee:	89 02                	mov    %eax,(%edx)
 80008f0:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 80008f4:	8b 55 0c             	mov    0xc(%ebp),%edx
 80008f7:	8b 45 14             	mov    0x14(%ebp),%eax
 80008fa:	01 d0                	add    %edx,%eax
 80008fc:	39 45 f0             	cmp    %eax,-0x10(%ebp)
 80008ff:	7c b1                	jl     80008b2 <_ZN8QPainter8fillRectEiiii+0xa0>
 8000901:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8000905:	8b 55 10             	mov    0x10(%ebp),%edx
 8000908:	8b 45 18             	mov    0x18(%ebp),%eax
 800090b:	01 d0                	add    %edx,%eax
 800090d:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8000910:	7c 84                	jl     8000896 <_ZN8QPainter8fillRectEiiii+0x84>
 8000912:	eb 01                	jmp    8000915 <_ZN8QPainter8fillRectEiiii+0x103>
 8000914:	90                   	nop
 8000915:	c9                   	leave
 8000916:	c3                   	ret
 8000917:	90                   	nop

08000918 <_ZN8QPainter8drawRectEiiii>:
 8000918:	55                   	push   %ebp
 8000919:	89 e5                	mov    %esp,%ebp
 800091b:	83 ec 18             	sub    $0x18,%esp
 800091e:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8000925:	eb 41                	jmp    8000968 <_ZN8QPainter8drawRectEiiii+0x50>
 8000927:	8b 55 0c             	mov    0xc(%ebp),%edx
 800092a:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800092d:	01 d0                	add    %edx,%eax
 800092f:	83 ec 04             	sub    $0x4,%esp
 8000932:	ff 75 10             	push   0x10(%ebp)
 8000935:	50                   	push   %eax
 8000936:	ff 75 08             	push   0x8(%ebp)
 8000939:	e8 68 fe ff ff       	call   80007a6 <_ZN8QPainter9drawPixelEii>
 800093e:	83 c4 10             	add    $0x10,%esp
 8000941:	8b 55 10             	mov    0x10(%ebp),%edx
 8000944:	8b 45 18             	mov    0x18(%ebp),%eax
 8000947:	01 d0                	add    %edx,%eax
 8000949:	8d 50 ff             	lea    -0x1(%eax),%edx
 800094c:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 800094f:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000952:	01 c8                	add    %ecx,%eax
 8000954:	83 ec 04             	sub    $0x4,%esp
 8000957:	52                   	push   %edx
 8000958:	50                   	push   %eax
 8000959:	ff 75 08             	push   0x8(%ebp)
 800095c:	e8 45 fe ff ff       	call   80007a6 <_ZN8QPainter9drawPixelEii>
 8000961:	83 c4 10             	add    $0x10,%esp
 8000964:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8000968:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800096b:	3b 45 14             	cmp    0x14(%ebp),%eax
 800096e:	7c b7                	jl     8000927 <_ZN8QPainter8drawRectEiiii+0xf>
 8000970:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8000977:	eb 41                	jmp    80009ba <_ZN8QPainter8drawRectEiiii+0xa2>
 8000979:	8b 55 10             	mov    0x10(%ebp),%edx
 800097c:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800097f:	01 d0                	add    %edx,%eax
 8000981:	83 ec 04             	sub    $0x4,%esp
 8000984:	50                   	push   %eax
 8000985:	ff 75 0c             	push   0xc(%ebp)
 8000988:	ff 75 08             	push   0x8(%ebp)
 800098b:	e8 16 fe ff ff       	call   80007a6 <_ZN8QPainter9drawPixelEii>
 8000990:	83 c4 10             	add    $0x10,%esp
 8000993:	8b 55 10             	mov    0x10(%ebp),%edx
 8000996:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000999:	01 c2                	add    %eax,%edx
 800099b:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 800099e:	8b 45 14             	mov    0x14(%ebp),%eax
 80009a1:	01 c8                	add    %ecx,%eax
 80009a3:	83 e8 01             	sub    $0x1,%eax
 80009a6:	83 ec 04             	sub    $0x4,%esp
 80009a9:	52                   	push   %edx
 80009aa:	50                   	push   %eax
 80009ab:	ff 75 08             	push   0x8(%ebp)
 80009ae:	e8 f3 fd ff ff       	call   80007a6 <_ZN8QPainter9drawPixelEii>
 80009b3:	83 c4 10             	add    $0x10,%esp
 80009b6:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 80009ba:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80009bd:	3b 45 18             	cmp    0x18(%ebp),%eax
 80009c0:	7c b7                	jl     8000979 <_ZN8QPainter8drawRectEiiii+0x61>
 80009c2:	90                   	nop
 80009c3:	90                   	nop
 80009c4:	c9                   	leave
 80009c5:	c3                   	ret

080009c6 <_ZN8QPainter8drawLineEiiii>:
 80009c6:	55                   	push   %ebp
 80009c7:	89 e5                	mov    %esp,%ebp
 80009c9:	83 ec 28             	sub    $0x28,%esp
 80009cc:	8b 45 14             	mov    0x14(%ebp),%eax
 80009cf:	2b 45 0c             	sub    0xc(%ebp),%eax
 80009d2:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80009d5:	8b 45 18             	mov    0x18(%ebp),%eax
 80009d8:	2b 45 10             	sub    0x10(%ebp),%eax
 80009db:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80009de:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80009e2:	7e 07                	jle    80009eb <_ZN8QPainter8drawLineEiiii+0x25>
 80009e4:	b8 01 00 00 00       	mov    $0x1,%eax
 80009e9:	eb 05                	jmp    80009f0 <_ZN8QPainter8drawLineEiiii+0x2a>
 80009eb:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80009f0:	89 45 e8             	mov    %eax,-0x18(%ebp)
 80009f3:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 80009f7:	7e 07                	jle    8000a00 <_ZN8QPainter8drawLineEiiii+0x3a>
 80009f9:	b8 01 00 00 00       	mov    $0x1,%eax
 80009fe:	eb 05                	jmp    8000a05 <_ZN8QPainter8drawLineEiiii+0x3f>
 8000a00:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8000a05:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 8000a08:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 8000a0c:	79 03                	jns    8000a11 <_ZN8QPainter8drawLineEiiii+0x4b>
 8000a0e:	f7 5d f4             	negl   -0xc(%ebp)
 8000a11:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 8000a15:	79 03                	jns    8000a1a <_ZN8QPainter8drawLineEiiii+0x54>
 8000a17:	f7 5d f0             	negl   -0x10(%ebp)
 8000a1a:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000a1d:	2b 45 f0             	sub    -0x10(%ebp),%eax
 8000a20:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8000a23:	83 ec 04             	sub    $0x4,%esp
 8000a26:	ff 75 10             	push   0x10(%ebp)
 8000a29:	ff 75 0c             	push   0xc(%ebp)
 8000a2c:	ff 75 08             	push   0x8(%ebp)
 8000a2f:	e8 72 fd ff ff       	call   80007a6 <_ZN8QPainter9drawPixelEii>
 8000a34:	83 c4 10             	add    $0x10,%esp
 8000a37:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000a3a:	3b 45 14             	cmp    0x14(%ebp),%eax
 8000a3d:	75 08                	jne    8000a47 <_ZN8QPainter8drawLineEiiii+0x81>
 8000a3f:	8b 45 10             	mov    0x10(%ebp),%eax
 8000a42:	3b 45 18             	cmp    0x18(%ebp),%eax
 8000a45:	74 34                	je     8000a7b <_ZN8QPainter8drawLineEiiii+0xb5>
 8000a47:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000a4a:	01 c0                	add    %eax,%eax
 8000a4c:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8000a4f:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000a52:	f7 d8                	neg    %eax
 8000a54:	39 45 e0             	cmp    %eax,-0x20(%ebp)
 8000a57:	7e 0c                	jle    8000a65 <_ZN8QPainter8drawLineEiiii+0x9f>
 8000a59:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000a5c:	29 45 ec             	sub    %eax,-0x14(%ebp)
 8000a5f:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8000a62:	01 45 0c             	add    %eax,0xc(%ebp)
 8000a65:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8000a68:	3b 45 f4             	cmp    -0xc(%ebp),%eax
 8000a6b:	7d b6                	jge    8000a23 <_ZN8QPainter8drawLineEiiii+0x5d>
 8000a6d:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000a70:	01 45 ec             	add    %eax,-0x14(%ebp)
 8000a73:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8000a76:	01 45 10             	add    %eax,0x10(%ebp)
 8000a79:	eb a8                	jmp    8000a23 <_ZN8QPainter8drawLineEiiii+0x5d>
 8000a7b:	90                   	nop
 8000a7c:	90                   	nop
 8000a7d:	c9                   	leave
 8000a7e:	c3                   	ret
 8000a7f:	90                   	nop

08000a80 <_ZN8QPainter8drawCharEiic>:
 8000a80:	55                   	push   %ebp
 8000a81:	89 e5                	mov    %esp,%ebp
 8000a83:	53                   	push   %ebx
 8000a84:	83 ec 24             	sub    $0x24,%esp
 8000a87:	8b 45 14             	mov    0x14(%ebp),%eax
 8000a8a:	88 45 e4             	mov    %al,-0x1c(%ebp)
 8000a8d:	80 7d e4 1f          	cmpb   $0x1f,-0x1c(%ebp)
 8000a91:	0f 8e 8d 00 00 00    	jle    8000b24 <_ZN8QPainter8drawCharEiic+0xa4>
 8000a97:	80 7d e4 7f          	cmpb   $0x7f,-0x1c(%ebp)
 8000a9b:	0f 84 83 00 00 00    	je     8000b24 <_ZN8QPainter8drawCharEiic+0xa4>
 8000aa1:	0f b6 45 e4          	movzbl -0x1c(%ebp),%eax
 8000aa5:	0f b6 c0             	movzbl %al,%eax
 8000aa8:	83 e8 20             	sub    $0x20,%eax
 8000aab:	c1 e0 03             	shl    $0x3,%eax
 8000aae:	05 40 4d 00 08       	add    $0x8004d40,%eax
 8000ab3:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8000ab6:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8000abd:	eb 5d                	jmp    8000b1c <_ZN8QPainter8drawCharEiic+0x9c>
 8000abf:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8000ac2:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000ac5:	01 d0                	add    %edx,%eax
 8000ac7:	0f b6 00             	movzbl (%eax),%eax
 8000aca:	88 45 eb             	mov    %al,-0x15(%ebp)
 8000acd:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8000ad4:	eb 3c                	jmp    8000b12 <_ZN8QPainter8drawCharEiic+0x92>
 8000ad6:	0f b6 55 eb          	movzbl -0x15(%ebp),%edx
 8000ada:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000add:	bb 80 00 00 00       	mov    $0x80,%ebx
 8000ae2:	89 c1                	mov    %eax,%ecx
 8000ae4:	d3 fb                	sar    %cl,%ebx
 8000ae6:	89 d8                	mov    %ebx,%eax
 8000ae8:	21 d0                	and    %edx,%eax
 8000aea:	85 c0                	test   %eax,%eax
 8000aec:	74 20                	je     8000b0e <_ZN8QPainter8drawCharEiic+0x8e>
 8000aee:	8b 55 10             	mov    0x10(%ebp),%edx
 8000af1:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000af4:	01 c2                	add    %eax,%edx
 8000af6:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8000af9:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000afc:	01 c8                	add    %ecx,%eax
 8000afe:	83 ec 04             	sub    $0x4,%esp
 8000b01:	52                   	push   %edx
 8000b02:	50                   	push   %eax
 8000b03:	ff 75 08             	push   0x8(%ebp)
 8000b06:	e8 9b fc ff ff       	call   80007a6 <_ZN8QPainter9drawPixelEii>
 8000b0b:	83 c4 10             	add    $0x10,%esp
 8000b0e:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 8000b12:	83 7d f0 07          	cmpl   $0x7,-0x10(%ebp)
 8000b16:	7e be                	jle    8000ad6 <_ZN8QPainter8drawCharEiic+0x56>
 8000b18:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8000b1c:	83 7d f4 07          	cmpl   $0x7,-0xc(%ebp)
 8000b20:	7e 9d                	jle    8000abf <_ZN8QPainter8drawCharEiic+0x3f>
 8000b22:	eb 01                	jmp    8000b25 <_ZN8QPainter8drawCharEiic+0xa5>
 8000b24:	90                   	nop
 8000b25:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8000b28:	c9                   	leave
 8000b29:	c3                   	ret

08000b2a <_ZN8QPainter8drawTextEiiPKc>:
 8000b2a:	55                   	push   %ebp
 8000b2b:	89 e5                	mov    %esp,%ebp
 8000b2d:	83 ec 18             	sub    $0x18,%esp
 8000b30:	83 7d 14 00          	cmpl   $0x0,0x14(%ebp)
 8000b34:	74 4d                	je     8000b83 <_ZN8QPainter8drawTextEiiPKc+0x59>
 8000b36:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000b39:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000b3c:	eb 39                	jmp    8000b77 <_ZN8QPainter8drawTextEiiPKc+0x4d>
 8000b3e:	8b 45 14             	mov    0x14(%ebp),%eax
 8000b41:	0f b6 00             	movzbl (%eax),%eax
 8000b44:	3c 0a                	cmp    $0xa,%al
 8000b46:	75 0c                	jne    8000b54 <_ZN8QPainter8drawTextEiiPKc+0x2a>
 8000b48:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000b4b:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000b4e:	83 45 10 08          	addl   $0x8,0x10(%ebp)
 8000b52:	eb 1f                	jmp    8000b73 <_ZN8QPainter8drawTextEiiPKc+0x49>
 8000b54:	8b 45 14             	mov    0x14(%ebp),%eax
 8000b57:	0f b6 00             	movzbl (%eax),%eax
 8000b5a:	0f be c0             	movsbl %al,%eax
 8000b5d:	50                   	push   %eax
 8000b5e:	ff 75 10             	push   0x10(%ebp)
 8000b61:	ff 75 f4             	push   -0xc(%ebp)
 8000b64:	ff 75 08             	push   0x8(%ebp)
 8000b67:	e8 14 ff ff ff       	call   8000a80 <_ZN8QPainter8drawCharEiic>
 8000b6c:	83 c4 10             	add    $0x10,%esp
 8000b6f:	83 45 f4 08          	addl   $0x8,-0xc(%ebp)
 8000b73:	83 45 14 01          	addl   $0x1,0x14(%ebp)
 8000b77:	8b 45 14             	mov    0x14(%ebp),%eax
 8000b7a:	0f b6 00             	movzbl (%eax),%eax
 8000b7d:	84 c0                	test   %al,%al
 8000b7f:	75 bd                	jne    8000b3e <_ZN8QPainter8drawTextEiiPKc+0x14>
 8000b81:	eb 01                	jmp    8000b84 <_ZN8QPainter8drawTextEiiPKc+0x5a>
 8000b83:	90                   	nop
 8000b84:	c9                   	leave
 8000b85:	c3                   	ret

08000b86 <_ZNK8QPainter9textWidthEPKc>:
 8000b86:	55                   	push   %ebp
 8000b87:	89 e5                	mov    %esp,%ebp
 8000b89:	83 ec 10             	sub    $0x10,%esp
 8000b8c:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8000b90:	75 07                	jne    8000b99 <_ZNK8QPainter9textWidthEPKc+0x13>
 8000b92:	b8 00 00 00 00       	mov    $0x0,%eax
 8000b97:	eb 28                	jmp    8000bc1 <_ZNK8QPainter9textWidthEPKc+0x3b>
 8000b99:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8000ba0:	eb 04                	jmp    8000ba6 <_ZNK8QPainter9textWidthEPKc+0x20>
 8000ba2:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 8000ba6:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000ba9:	8d 50 01             	lea    0x1(%eax),%edx
 8000bac:	89 55 0c             	mov    %edx,0xc(%ebp)
 8000baf:	0f b6 00             	movzbl (%eax),%eax
 8000bb2:	84 c0                	test   %al,%al
 8000bb4:	0f 95 c0             	setne  %al
 8000bb7:	84 c0                	test   %al,%al
 8000bb9:	75 e7                	jne    8000ba2 <_ZNK8QPainter9textWidthEPKc+0x1c>
 8000bbb:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8000bbe:	c1 e0 03             	shl    $0x3,%eax
 8000bc1:	c9                   	leave
 8000bc2:	c3                   	ret
 8000bc3:	90                   	nop

08000bc4 <_ZN8QPainter5clearEj>:
 8000bc4:	55                   	push   %ebp
 8000bc5:	89 e5                	mov    %esp,%ebp
 8000bc7:	83 ec 10             	sub    $0x10,%esp
 8000bca:	8b 45 08             	mov    0x8(%ebp),%eax
 8000bcd:	8b 50 10             	mov    0x10(%eax),%edx
 8000bd0:	8b 45 08             	mov    0x8(%ebp),%eax
 8000bd3:	8b 40 08             	mov    0x8(%eax),%eax
 8000bd6:	0f af c2             	imul   %edx,%eax
 8000bd9:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8000bdc:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8000be3:	eb 16                	jmp    8000bfb <_ZN8QPainter5clearEj+0x37>
 8000be5:	8b 45 08             	mov    0x8(%ebp),%eax
 8000be8:	8b 00                	mov    (%eax),%eax
 8000bea:	8b 55 fc             	mov    -0x4(%ebp),%edx
 8000bed:	c1 e2 02             	shl    $0x2,%edx
 8000bf0:	01 c2                	add    %eax,%edx
 8000bf2:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000bf5:	89 02                	mov    %eax,(%edx)
 8000bf7:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 8000bfb:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8000bfe:	3b 45 f8             	cmp    -0x8(%ebp),%eax
 8000c01:	7c e2                	jl     8000be5 <_ZN8QPainter5clearEj+0x21>
 8000c03:	90                   	nop
 8000c04:	90                   	nop
 8000c05:	c9                   	leave
 8000c06:	c3                   	ret
 8000c07:	90                   	nop

08000c08 <_ZN8QPainter11setClipRectEiiii>:
 8000c08:	55                   	push   %ebp
 8000c09:	89 e5                	mov    %esp,%ebp
 8000c0b:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c0e:	c6 40 18 01          	movb   $0x1,0x18(%eax)
 8000c12:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c15:	8b 55 0c             	mov    0xc(%ebp),%edx
 8000c18:	89 50 1c             	mov    %edx,0x1c(%eax)
 8000c1b:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c1e:	8b 55 10             	mov    0x10(%ebp),%edx
 8000c21:	89 50 20             	mov    %edx,0x20(%eax)
 8000c24:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c27:	8b 55 14             	mov    0x14(%ebp),%edx
 8000c2a:	89 50 24             	mov    %edx,0x24(%eax)
 8000c2d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c30:	8b 55 18             	mov    0x18(%ebp),%edx
 8000c33:	89 50 28             	mov    %edx,0x28(%eax)
 8000c36:	90                   	nop
 8000c37:	5d                   	pop    %ebp
 8000c38:	c3                   	ret
 8000c39:	90                   	nop

08000c3a <_ZN8QPainter9clearClipEv>:
 8000c3a:	55                   	push   %ebp
 8000c3b:	89 e5                	mov    %esp,%ebp
 8000c3d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c40:	c6 40 18 00          	movb   $0x0,0x18(%eax)
 8000c44:	90                   	nop
 8000c45:	5d                   	pop    %ebp
 8000c46:	c3                   	ret
 8000c47:	90                   	nop

08000c48 <_ZNK8QPainter9isClippedEii>:
 8000c48:	55                   	push   %ebp
 8000c49:	89 e5                	mov    %esp,%ebp
 8000c4b:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c4e:	0f b6 40 18          	movzbl 0x18(%eax),%eax
 8000c52:	83 f0 01             	xor    $0x1,%eax
 8000c55:	84 c0                	test   %al,%al
 8000c57:	74 07                	je     8000c60 <_ZNK8QPainter9isClippedEii+0x18>
 8000c59:	b8 00 00 00 00       	mov    $0x0,%eax
 8000c5e:	eb 49                	jmp    8000ca9 <_ZNK8QPainter9isClippedEii+0x61>
 8000c60:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c63:	8b 40 1c             	mov    0x1c(%eax),%eax
 8000c66:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8000c69:	7c 31                	jl     8000c9c <_ZNK8QPainter9isClippedEii+0x54>
 8000c6b:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c6e:	8b 50 1c             	mov    0x1c(%eax),%edx
 8000c71:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c74:	8b 40 24             	mov    0x24(%eax),%eax
 8000c77:	01 d0                	add    %edx,%eax
 8000c79:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8000c7c:	7d 1e                	jge    8000c9c <_ZNK8QPainter9isClippedEii+0x54>
 8000c7e:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c81:	8b 40 20             	mov    0x20(%eax),%eax
 8000c84:	39 45 10             	cmp    %eax,0x10(%ebp)
 8000c87:	7c 13                	jl     8000c9c <_ZNK8QPainter9isClippedEii+0x54>
 8000c89:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c8c:	8b 50 20             	mov    0x20(%eax),%edx
 8000c8f:	8b 45 08             	mov    0x8(%ebp),%eax
 8000c92:	8b 40 28             	mov    0x28(%eax),%eax
 8000c95:	01 d0                	add    %edx,%eax
 8000c97:	39 45 10             	cmp    %eax,0x10(%ebp)
 8000c9a:	7c 07                	jl     8000ca3 <_ZNK8QPainter9isClippedEii+0x5b>
 8000c9c:	b8 01 00 00 00       	mov    $0x1,%eax
 8000ca1:	eb 05                	jmp    8000ca8 <_ZNK8QPainter9isClippedEii+0x60>
 8000ca3:	b8 00 00 00 00       	mov    $0x0,%eax
 8000ca8:	90                   	nop
 8000ca9:	5d                   	pop    %ebp
 8000caa:	c3                   	ret
 8000cab:	90                   	nop

08000cac <_ZL13strdup_simplePKc>:
 8000cac:	55                   	push   %ebp
 8000cad:	89 e5                	mov    %esp,%ebp
 8000caf:	83 ec 18             	sub    $0x18,%esp
 8000cb2:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 8000cb6:	75 07                	jne    8000cbf <_ZL13strdup_simplePKc+0x13>
 8000cb8:	b8 00 00 00 00       	mov    $0x0,%eax
 8000cbd:	eb 5e                	jmp    8000d1d <_ZL13strdup_simplePKc+0x71>
 8000cbf:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8000cc6:	eb 04                	jmp    8000ccc <_ZL13strdup_simplePKc+0x20>
 8000cc8:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8000ccc:	8b 55 08             	mov    0x8(%ebp),%edx
 8000ccf:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000cd2:	01 d0                	add    %edx,%eax
 8000cd4:	0f b6 00             	movzbl (%eax),%eax
 8000cd7:	84 c0                	test   %al,%al
 8000cd9:	75 ed                	jne    8000cc8 <_ZL13strdup_simplePKc+0x1c>
 8000cdb:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000cde:	83 c0 01             	add    $0x1,%eax
 8000ce1:	83 ec 0c             	sub    $0xc,%esp
 8000ce4:	50                   	push   %eax
 8000ce5:	e8 f2 f3 ff ff       	call   80000dc <_Znaj>
 8000cea:	83 c4 10             	add    $0x10,%esp
 8000ced:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8000cf0:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8000cf7:	eb 19                	jmp    8000d12 <_ZL13strdup_simplePKc+0x66>
 8000cf9:	8b 55 08             	mov    0x8(%ebp),%edx
 8000cfc:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000cff:	01 d0                	add    %edx,%eax
 8000d01:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 8000d04:	8b 55 f0             	mov    -0x10(%ebp),%edx
 8000d07:	01 ca                	add    %ecx,%edx
 8000d09:	0f b6 00             	movzbl (%eax),%eax
 8000d0c:	88 02                	mov    %al,(%edx)
 8000d0e:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 8000d12:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000d15:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8000d18:	73 df                	jae    8000cf9 <_ZL13strdup_simplePKc+0x4d>
 8000d1a:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000d1d:	c9                   	leave
 8000d1e:	c3                   	ret
 8000d1f:	90                   	nop

08000d20 <_ZN7QWidgetC1EPS_PKc>:
 8000d20:	55                   	push   %ebp
 8000d21:	89 e5                	mov    %esp,%ebp
 8000d23:	83 ec 08             	sub    $0x8,%esp
 8000d26:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d29:	83 ec 04             	sub    $0x4,%esp
 8000d2c:	ff 75 10             	push   0x10(%ebp)
 8000d2f:	ff 75 0c             	push   0xc(%ebp)
 8000d32:	50                   	push   %eax
 8000d33:	e8 00 f5 ff ff       	call   8000238 <_ZN7QObjectC1EPS_PKc>
 8000d38:	83 c4 10             	add    $0x10,%esp
 8000d3b:	ba 94 50 00 08       	mov    $0x8005094,%edx
 8000d40:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d43:	89 10                	mov    %edx,(%eax)
 8000d45:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d48:	c7 40 1c 00 00 00 00 	movl   $0x0,0x1c(%eax)
 8000d4f:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d52:	c7 40 20 00 00 00 00 	movl   $0x0,0x20(%eax)
 8000d59:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d5c:	c7 40 24 64 00 00 00 	movl   $0x64,0x24(%eax)
 8000d63:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d66:	c7 40 28 1e 00 00 00 	movl   $0x1e,0x28(%eax)
 8000d6d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d70:	c6 40 2c 01          	movb   $0x1,0x2c(%eax)
 8000d74:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d77:	c7 40 30 c0 c0 c0 00 	movl   $0xc0c0c0,0x30(%eax)
 8000d7e:	90                   	nop
 8000d7f:	c9                   	leave
 8000d80:	c3                   	ret
 8000d81:	90                   	nop

08000d82 <_ZN7QWidgetD1Ev>:
 8000d82:	55                   	push   %ebp
 8000d83:	89 e5                	mov    %esp,%ebp
 8000d85:	83 ec 08             	sub    $0x8,%esp
 8000d88:	ba 94 50 00 08       	mov    $0x8005094,%edx
 8000d8d:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d90:	89 10                	mov    %edx,(%eax)
 8000d92:	8b 45 08             	mov    0x8(%ebp),%eax
 8000d95:	83 ec 0c             	sub    $0xc,%esp
 8000d98:	50                   	push   %eax
 8000d99:	e8 14 f5 ff ff       	call   80002b2 <_ZN7QObjectD1Ev>
 8000d9e:	83 c4 10             	add    $0x10,%esp
 8000da1:	90                   	nop
 8000da2:	c9                   	leave
 8000da3:	c3                   	ret

08000da4 <_ZN7QWidgetD0Ev>:
 8000da4:	55                   	push   %ebp
 8000da5:	89 e5                	mov    %esp,%ebp
 8000da7:	83 ec 08             	sub    $0x8,%esp
 8000daa:	83 ec 0c             	sub    $0xc,%esp
 8000dad:	ff 75 08             	push   0x8(%ebp)
 8000db0:	e8 cd ff ff ff       	call   8000d82 <_ZN7QWidgetD1Ev>
 8000db5:	83 c4 10             	add    $0x10,%esp
 8000db8:	83 ec 08             	sub    $0x8,%esp
 8000dbb:	6a 34                	push   $0x34
 8000dbd:	ff 75 08             	push   0x8(%ebp)
 8000dc0:	e8 39 f3 ff ff       	call   80000fe <_ZdlPvj>
 8000dc5:	83 c4 10             	add    $0x10,%esp
 8000dc8:	c9                   	leave
 8000dc9:	c3                   	ret

08000dca <_ZN7QWidget11setGeometryEiiii>:
 8000dca:	55                   	push   %ebp
 8000dcb:	89 e5                	mov    %esp,%ebp
 8000dcd:	8b 45 08             	mov    0x8(%ebp),%eax
 8000dd0:	8b 55 0c             	mov    0xc(%ebp),%edx
 8000dd3:	89 50 1c             	mov    %edx,0x1c(%eax)
 8000dd6:	8b 45 08             	mov    0x8(%ebp),%eax
 8000dd9:	8b 55 10             	mov    0x10(%ebp),%edx
 8000ddc:	89 50 20             	mov    %edx,0x20(%eax)
 8000ddf:	8b 45 08             	mov    0x8(%ebp),%eax
 8000de2:	8b 55 14             	mov    0x14(%ebp),%edx
 8000de5:	89 50 24             	mov    %edx,0x24(%eax)
 8000de8:	8b 45 08             	mov    0x8(%ebp),%eax
 8000deb:	8b 55 18             	mov    0x18(%ebp),%edx
 8000dee:	89 50 28             	mov    %edx,0x28(%eax)
 8000df1:	90                   	nop
 8000df2:	5d                   	pop    %ebp
 8000df3:	c3                   	ret

08000df4 <_ZN7QWidget10setVisibleEb>:
 8000df4:	55                   	push   %ebp
 8000df5:	89 e5                	mov    %esp,%ebp
 8000df7:	83 ec 04             	sub    $0x4,%esp
 8000dfa:	8b 45 0c             	mov    0xc(%ebp),%eax
 8000dfd:	88 45 fc             	mov    %al,-0x4(%ebp)
 8000e00:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e03:	0f b6 55 fc          	movzbl -0x4(%ebp),%edx
 8000e07:	88 50 2c             	mov    %dl,0x2c(%eax)
 8000e0a:	90                   	nop
 8000e0b:	c9                   	leave
 8000e0c:	c3                   	ret
 8000e0d:	90                   	nop

08000e0e <_ZNK7QWidget10childCountEv>:
 8000e0e:	55                   	push   %ebp
 8000e0f:	89 e5                	mov    %esp,%ebp
 8000e11:	83 ec 18             	sub    $0x18,%esp
 8000e14:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8000e1b:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e1e:	83 ec 0c             	sub    $0xc,%esp
 8000e21:	50                   	push   %eax
 8000e22:	e8 67 07 00 00       	call   800158e <_ZNK7QObject10firstChildEv>
 8000e27:	83 c4 10             	add    $0x10,%esp
 8000e2a:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8000e2d:	eb 15                	jmp    8000e44 <_ZNK7QWidget10childCountEv+0x36>
 8000e2f:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8000e33:	83 ec 0c             	sub    $0xc,%esp
 8000e36:	ff 75 f0             	push   -0x10(%ebp)
 8000e39:	e8 5c 07 00 00       	call   800159a <_ZNK7QObject11nextSiblingEv>
 8000e3e:	83 c4 10             	add    $0x10,%esp
 8000e41:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8000e44:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 8000e48:	75 e5                	jne    8000e2f <_ZNK7QWidget10childCountEv+0x21>
 8000e4a:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000e4d:	c9                   	leave
 8000e4e:	c3                   	ret
 8000e4f:	90                   	nop

08000e50 <_ZN7QWidget10paintEventEP8QPainter>:
 8000e50:	55                   	push   %ebp
 8000e51:	89 e5                	mov    %esp,%ebp
 8000e53:	53                   	push   %ebx
 8000e54:	83 ec 04             	sub    $0x4,%esp
 8000e57:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e5a:	8b 40 30             	mov    0x30(%eax),%eax
 8000e5d:	83 ec 08             	sub    $0x8,%esp
 8000e60:	50                   	push   %eax
 8000e61:	ff 75 0c             	push   0xc(%ebp)
 8000e64:	e8 2d f9 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8000e69:	83 c4 10             	add    $0x10,%esp
 8000e6c:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e6f:	8b 58 28             	mov    0x28(%eax),%ebx
 8000e72:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e75:	8b 48 24             	mov    0x24(%eax),%ecx
 8000e78:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e7b:	8b 50 20             	mov    0x20(%eax),%edx
 8000e7e:	8b 45 08             	mov    0x8(%ebp),%eax
 8000e81:	8b 40 1c             	mov    0x1c(%eax),%eax
 8000e84:	83 ec 0c             	sub    $0xc,%esp
 8000e87:	53                   	push   %ebx
 8000e88:	51                   	push   %ecx
 8000e89:	52                   	push   %edx
 8000e8a:	50                   	push   %eax
 8000e8b:	ff 75 0c             	push   0xc(%ebp)
 8000e8e:	e8 7f f9 ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 8000e93:	83 c4 20             	add    $0x20,%esp
 8000e96:	90                   	nop
 8000e97:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8000e9a:	c9                   	leave
 8000e9b:	c3                   	ret

08000e9c <_ZN7QWidget6renderEP8QPainterii>:
 8000e9c:	55                   	push   %ebp
 8000e9d:	89 e5                	mov    %esp,%ebp
 8000e9f:	83 ec 38             	sub    $0x38,%esp
 8000ea2:	8b 45 08             	mov    0x8(%ebp),%eax
 8000ea5:	0f b6 40 2c          	movzbl 0x2c(%eax),%eax
 8000ea9:	83 f0 01             	xor    $0x1,%eax
 8000eac:	84 c0                	test   %al,%al
 8000eae:	0f 85 3a 01 00 00    	jne    8000fee <_ZN7QWidget6renderEP8QPainterii+0x152>
 8000eb4:	8b 45 08             	mov    0x8(%ebp),%eax
 8000eb7:	8b 40 1c             	mov    0x1c(%eax),%eax
 8000eba:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8000ebd:	8b 45 08             	mov    0x8(%ebp),%eax
 8000ec0:	8b 40 20             	mov    0x20(%eax),%eax
 8000ec3:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 8000ec6:	8b 45 08             	mov    0x8(%ebp),%eax
 8000ec9:	8b 50 1c             	mov    0x1c(%eax),%edx
 8000ecc:	8b 45 10             	mov    0x10(%ebp),%eax
 8000ecf:	01 c2                	add    %eax,%edx
 8000ed1:	8b 45 08             	mov    0x8(%ebp),%eax
 8000ed4:	89 50 1c             	mov    %edx,0x1c(%eax)
 8000ed7:	8b 45 08             	mov    0x8(%ebp),%eax
 8000eda:	8b 50 20             	mov    0x20(%eax),%edx
 8000edd:	8b 45 14             	mov    0x14(%ebp),%eax
 8000ee0:	01 c2                	add    %eax,%edx
 8000ee2:	8b 45 08             	mov    0x8(%ebp),%eax
 8000ee5:	89 50 20             	mov    %edx,0x20(%eax)
 8000ee8:	8b 45 08             	mov    0x8(%ebp),%eax
 8000eeb:	8b 00                	mov    (%eax),%eax
 8000eed:	83 c0 0c             	add    $0xc,%eax
 8000ef0:	8b 00                	mov    (%eax),%eax
 8000ef2:	83 ec 08             	sub    $0x8,%esp
 8000ef5:	ff 75 0c             	push   0xc(%ebp)
 8000ef8:	ff 75 08             	push   0x8(%ebp)
 8000efb:	ff d0                	call   *%eax
 8000efd:	83 c4 10             	add    $0x10,%esp
 8000f00:	83 ec 0c             	sub    $0xc,%esp
 8000f03:	ff 75 08             	push   0x8(%ebp)
 8000f06:	e8 03 ff ff ff       	call   8000e0e <_ZNK7QWidget10childCountEv>
 8000f0b:	83 c4 10             	add    $0x10,%esp
 8000f0e:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8000f11:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
 8000f15:	0f 8e bf 00 00 00    	jle    8000fda <_ZN7QWidget6renderEP8QPainterii+0x13e>
 8000f1b:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8000f1e:	c1 e0 02             	shl    $0x2,%eax
 8000f21:	83 ec 0c             	sub    $0xc,%esp
 8000f24:	50                   	push   %eax
 8000f25:	e8 b2 f1 ff ff       	call   80000dc <_Znaj>
 8000f2a:	83 c4 10             	add    $0x10,%esp
 8000f2d:	89 45 dc             	mov    %eax,-0x24(%ebp)
 8000f30:	8b 45 08             	mov    0x8(%ebp),%eax
 8000f33:	83 ec 0c             	sub    $0xc,%esp
 8000f36:	50                   	push   %eax
 8000f37:	e8 52 06 00 00       	call   800158e <_ZNK7QObject10firstChildEv>
 8000f3c:	83 c4 10             	add    $0x10,%esp
 8000f3f:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000f42:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8000f45:	83 e8 01             	sub    $0x1,%eax
 8000f48:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8000f4b:	eb 29                	jmp    8000f76 <_ZN7QWidget6renderEP8QPainterii+0xda>
 8000f4d:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8000f50:	8d 14 85 00 00 00 00 	lea    0x0(,%eax,4),%edx
 8000f57:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8000f5a:	01 c2                	add    %eax,%edx
 8000f5c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8000f5f:	89 02                	mov    %eax,(%edx)
 8000f61:	83 ec 0c             	sub    $0xc,%esp
 8000f64:	ff 75 f4             	push   -0xc(%ebp)
 8000f67:	e8 2e 06 00 00       	call   800159a <_ZNK7QObject11nextSiblingEv>
 8000f6c:	83 c4 10             	add    $0x10,%esp
 8000f6f:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8000f72:	83 6d f0 01          	subl   $0x1,-0x10(%ebp)
 8000f76:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 8000f7a:	79 d1                	jns    8000f4d <_ZN7QWidget6renderEP8QPainterii+0xb1>
 8000f7c:	8b 45 08             	mov    0x8(%ebp),%eax
 8000f7f:	8b 40 1c             	mov    0x1c(%eax),%eax
 8000f82:	89 45 d8             	mov    %eax,-0x28(%ebp)
 8000f85:	8b 45 08             	mov    0x8(%ebp),%eax
 8000f88:	8b 40 20             	mov    0x20(%eax),%eax
 8000f8b:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 8000f8e:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
 8000f95:	eb 27                	jmp    8000fbe <_ZN7QWidget6renderEP8QPainterii+0x122>
 8000f97:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000f9a:	8d 14 85 00 00 00 00 	lea    0x0(,%eax,4),%edx
 8000fa1:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8000fa4:	01 d0                	add    %edx,%eax
 8000fa6:	8b 00                	mov    (%eax),%eax
 8000fa8:	ff 75 d4             	push   -0x2c(%ebp)
 8000fab:	ff 75 d8             	push   -0x28(%ebp)
 8000fae:	ff 75 0c             	push   0xc(%ebp)
 8000fb1:	50                   	push   %eax
 8000fb2:	e8 e5 fe ff ff       	call   8000e9c <_ZN7QWidget6renderEP8QPainterii>
 8000fb7:	83 c4 10             	add    $0x10,%esp
 8000fba:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
 8000fbe:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8000fc1:	3b 45 e0             	cmp    -0x20(%ebp),%eax
 8000fc4:	7c d1                	jl     8000f97 <_ZN7QWidget6renderEP8QPainterii+0xfb>
 8000fc6:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 8000fca:	74 0e                	je     8000fda <_ZN7QWidget6renderEP8QPainterii+0x13e>
 8000fcc:	83 ec 0c             	sub    $0xc,%esp
 8000fcf:	ff 75 dc             	push   -0x24(%ebp)
 8000fd2:	e8 21 f1 ff ff       	call   80000f8 <_ZdaPv>
 8000fd7:	83 c4 10             	add    $0x10,%esp
 8000fda:	8b 45 08             	mov    0x8(%ebp),%eax
 8000fdd:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8000fe0:	89 50 1c             	mov    %edx,0x1c(%eax)
 8000fe3:	8b 45 08             	mov    0x8(%ebp),%eax
 8000fe6:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 8000fe9:	89 50 20             	mov    %edx,0x20(%eax)
 8000fec:	eb 01                	jmp    8000fef <_ZN7QWidget6renderEP8QPainterii+0x153>
 8000fee:	90                   	nop
 8000fef:	c9                   	leave
 8000ff0:	c3                   	ret
 8000ff1:	90                   	nop

08000ff2 <_ZNK7QWidget8containsEii>:
 8000ff2:	55                   	push   %ebp
 8000ff3:	89 e5                	mov    %esp,%ebp
 8000ff5:	8b 45 08             	mov    0x8(%ebp),%eax
 8000ff8:	8b 40 1c             	mov    0x1c(%eax),%eax
 8000ffb:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8000ffe:	7c 38                	jl     8001038 <_ZNK7QWidget8containsEii+0x46>
 8001000:	8b 45 08             	mov    0x8(%ebp),%eax
 8001003:	8b 50 1c             	mov    0x1c(%eax),%edx
 8001006:	8b 45 08             	mov    0x8(%ebp),%eax
 8001009:	8b 40 24             	mov    0x24(%eax),%eax
 800100c:	01 d0                	add    %edx,%eax
 800100e:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001011:	7d 25                	jge    8001038 <_ZNK7QWidget8containsEii+0x46>
 8001013:	8b 45 08             	mov    0x8(%ebp),%eax
 8001016:	8b 40 20             	mov    0x20(%eax),%eax
 8001019:	39 45 10             	cmp    %eax,0x10(%ebp)
 800101c:	7c 1a                	jl     8001038 <_ZNK7QWidget8containsEii+0x46>
 800101e:	8b 45 08             	mov    0x8(%ebp),%eax
 8001021:	8b 50 20             	mov    0x20(%eax),%edx
 8001024:	8b 45 08             	mov    0x8(%ebp),%eax
 8001027:	8b 40 28             	mov    0x28(%eax),%eax
 800102a:	01 d0                	add    %edx,%eax
 800102c:	39 45 10             	cmp    %eax,0x10(%ebp)
 800102f:	7d 07                	jge    8001038 <_ZNK7QWidget8containsEii+0x46>
 8001031:	b8 01 00 00 00       	mov    $0x1,%eax
 8001036:	eb 05                	jmp    800103d <_ZNK7QWidget8containsEii+0x4b>
 8001038:	b8 00 00 00 00       	mov    $0x0,%eax
 800103d:	5d                   	pop    %ebp
 800103e:	c3                   	ret
 800103f:	90                   	nop

08001040 <_ZN6QLabelC1EP7QWidgetPKcS3_>:
 8001040:	55                   	push   %ebp
 8001041:	89 e5                	mov    %esp,%ebp
 8001043:	83 ec 08             	sub    $0x8,%esp
 8001046:	8b 45 08             	mov    0x8(%ebp),%eax
 8001049:	83 ec 04             	sub    $0x4,%esp
 800104c:	ff 75 10             	push   0x10(%ebp)
 800104f:	ff 75 0c             	push   0xc(%ebp)
 8001052:	50                   	push   %eax
 8001053:	e8 c8 fc ff ff       	call   8000d20 <_ZN7QWidgetC1EPS_PKc>
 8001058:	83 c4 10             	add    $0x10,%esp
 800105b:	ba 7c 50 00 08       	mov    $0x800507c,%edx
 8001060:	8b 45 08             	mov    0x8(%ebp),%eax
 8001063:	89 10                	mov    %edx,(%eax)
 8001065:	8b 45 08             	mov    0x8(%ebp),%eax
 8001068:	c7 40 34 00 00 00 00 	movl   $0x0,0x34(%eax)
 800106f:	83 ec 0c             	sub    $0xc,%esp
 8001072:	ff 75 14             	push   0x14(%ebp)
 8001075:	e8 32 fc ff ff       	call   8000cac <_ZL13strdup_simplePKc>
 800107a:	83 c4 10             	add    $0x10,%esp
 800107d:	8b 55 08             	mov    0x8(%ebp),%edx
 8001080:	89 42 34             	mov    %eax,0x34(%edx)
 8001083:	8b 45 08             	mov    0x8(%ebp),%eax
 8001086:	c7 40 30 c0 c0 c0 00 	movl   $0xc0c0c0,0x30(%eax)
 800108d:	8b 45 08             	mov    0x8(%ebp),%eax
 8001090:	c7 40 24 78 00 00 00 	movl   $0x78,0x24(%eax)
 8001097:	8b 45 08             	mov    0x8(%ebp),%eax
 800109a:	c7 40 28 14 00 00 00 	movl   $0x14,0x28(%eax)
 80010a1:	90                   	nop
 80010a2:	c9                   	leave
 80010a3:	c3                   	ret

080010a4 <_ZN6QLabelD1Ev>:
 80010a4:	55                   	push   %ebp
 80010a5:	89 e5                	mov    %esp,%ebp
 80010a7:	83 ec 08             	sub    $0x8,%esp
 80010aa:	ba 7c 50 00 08       	mov    $0x800507c,%edx
 80010af:	8b 45 08             	mov    0x8(%ebp),%eax
 80010b2:	89 10                	mov    %edx,(%eax)
 80010b4:	8b 45 08             	mov    0x8(%ebp),%eax
 80010b7:	8b 40 34             	mov    0x34(%eax),%eax
 80010ba:	85 c0                	test   %eax,%eax
 80010bc:	74 1c                	je     80010da <_ZN6QLabelD1Ev+0x36>
 80010be:	8b 45 08             	mov    0x8(%ebp),%eax
 80010c1:	8b 40 34             	mov    0x34(%eax),%eax
 80010c4:	85 c0                	test   %eax,%eax
 80010c6:	74 12                	je     80010da <_ZN6QLabelD1Ev+0x36>
 80010c8:	8b 45 08             	mov    0x8(%ebp),%eax
 80010cb:	8b 40 34             	mov    0x34(%eax),%eax
 80010ce:	83 ec 0c             	sub    $0xc,%esp
 80010d1:	50                   	push   %eax
 80010d2:	e8 21 f0 ff ff       	call   80000f8 <_ZdaPv>
 80010d7:	83 c4 10             	add    $0x10,%esp
 80010da:	8b 45 08             	mov    0x8(%ebp),%eax
 80010dd:	83 ec 0c             	sub    $0xc,%esp
 80010e0:	50                   	push   %eax
 80010e1:	e8 9c fc ff ff       	call   8000d82 <_ZN7QWidgetD1Ev>
 80010e6:	83 c4 10             	add    $0x10,%esp
 80010e9:	90                   	nop
 80010ea:	c9                   	leave
 80010eb:	c3                   	ret

080010ec <_ZN6QLabelD0Ev>:
 80010ec:	55                   	push   %ebp
 80010ed:	89 e5                	mov    %esp,%ebp
 80010ef:	83 ec 08             	sub    $0x8,%esp
 80010f2:	83 ec 0c             	sub    $0xc,%esp
 80010f5:	ff 75 08             	push   0x8(%ebp)
 80010f8:	e8 a7 ff ff ff       	call   80010a4 <_ZN6QLabelD1Ev>
 80010fd:	83 c4 10             	add    $0x10,%esp
 8001100:	83 ec 08             	sub    $0x8,%esp
 8001103:	6a 38                	push   $0x38
 8001105:	ff 75 08             	push   0x8(%ebp)
 8001108:	e8 f1 ef ff ff       	call   80000fe <_ZdlPvj>
 800110d:	83 c4 10             	add    $0x10,%esp
 8001110:	c9                   	leave
 8001111:	c3                   	ret

08001112 <_ZN6QLabel7setTextEPKc>:
 8001112:	55                   	push   %ebp
 8001113:	89 e5                	mov    %esp,%ebp
 8001115:	83 ec 08             	sub    $0x8,%esp
 8001118:	8b 45 08             	mov    0x8(%ebp),%eax
 800111b:	8b 40 34             	mov    0x34(%eax),%eax
 800111e:	85 c0                	test   %eax,%eax
 8001120:	74 1c                	je     800113e <_ZN6QLabel7setTextEPKc+0x2c>
 8001122:	8b 45 08             	mov    0x8(%ebp),%eax
 8001125:	8b 40 34             	mov    0x34(%eax),%eax
 8001128:	85 c0                	test   %eax,%eax
 800112a:	74 12                	je     800113e <_ZN6QLabel7setTextEPKc+0x2c>
 800112c:	8b 45 08             	mov    0x8(%ebp),%eax
 800112f:	8b 40 34             	mov    0x34(%eax),%eax
 8001132:	83 ec 0c             	sub    $0xc,%esp
 8001135:	50                   	push   %eax
 8001136:	e8 bd ef ff ff       	call   80000f8 <_ZdaPv>
 800113b:	83 c4 10             	add    $0x10,%esp
 800113e:	83 ec 0c             	sub    $0xc,%esp
 8001141:	ff 75 0c             	push   0xc(%ebp)
 8001144:	e8 63 fb ff ff       	call   8000cac <_ZL13strdup_simplePKc>
 8001149:	83 c4 10             	add    $0x10,%esp
 800114c:	8b 55 08             	mov    0x8(%ebp),%edx
 800114f:	89 42 34             	mov    %eax,0x34(%edx)
 8001152:	90                   	nop
 8001153:	c9                   	leave
 8001154:	c3                   	ret
 8001155:	90                   	nop

08001156 <_ZN6QLabel10paintEventEP8QPainter>:
 8001156:	55                   	push   %ebp
 8001157:	89 e5                	mov    %esp,%ebp
 8001159:	53                   	push   %ebx
 800115a:	83 ec 04             	sub    $0x4,%esp
 800115d:	8b 45 08             	mov    0x8(%ebp),%eax
 8001160:	8b 40 30             	mov    0x30(%eax),%eax
 8001163:	83 ec 08             	sub    $0x8,%esp
 8001166:	50                   	push   %eax
 8001167:	ff 75 0c             	push   0xc(%ebp)
 800116a:	e8 27 f6 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 800116f:	83 c4 10             	add    $0x10,%esp
 8001172:	8b 45 08             	mov    0x8(%ebp),%eax
 8001175:	8b 58 28             	mov    0x28(%eax),%ebx
 8001178:	8b 45 08             	mov    0x8(%ebp),%eax
 800117b:	8b 48 24             	mov    0x24(%eax),%ecx
 800117e:	8b 45 08             	mov    0x8(%ebp),%eax
 8001181:	8b 50 20             	mov    0x20(%eax),%edx
 8001184:	8b 45 08             	mov    0x8(%ebp),%eax
 8001187:	8b 40 1c             	mov    0x1c(%eax),%eax
 800118a:	83 ec 0c             	sub    $0xc,%esp
 800118d:	53                   	push   %ebx
 800118e:	51                   	push   %ecx
 800118f:	52                   	push   %edx
 8001190:	50                   	push   %eax
 8001191:	ff 75 0c             	push   0xc(%ebp)
 8001194:	e8 79 f6 ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 8001199:	83 c4 20             	add    $0x20,%esp
 800119c:	8b 45 08             	mov    0x8(%ebp),%eax
 800119f:	8b 40 34             	mov    0x34(%eax),%eax
 80011a2:	85 c0                	test   %eax,%eax
 80011a4:	74 36                	je     80011dc <_ZN6QLabel10paintEventEP8QPainter+0x86>
 80011a6:	83 ec 08             	sub    $0x8,%esp
 80011a9:	6a 00                	push   $0x0
 80011ab:	ff 75 0c             	push   0xc(%ebp)
 80011ae:	e8 e3 f5 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 80011b3:	83 c4 10             	add    $0x10,%esp
 80011b6:	8b 45 08             	mov    0x8(%ebp),%eax
 80011b9:	8b 40 34             	mov    0x34(%eax),%eax
 80011bc:	8b 55 08             	mov    0x8(%ebp),%edx
 80011bf:	8b 52 20             	mov    0x20(%edx),%edx
 80011c2:	8d 4a 06             	lea    0x6(%edx),%ecx
 80011c5:	8b 55 08             	mov    0x8(%ebp),%edx
 80011c8:	8b 52 1c             	mov    0x1c(%edx),%edx
 80011cb:	83 c2 04             	add    $0x4,%edx
 80011ce:	50                   	push   %eax
 80011cf:	51                   	push   %ecx
 80011d0:	52                   	push   %edx
 80011d1:	ff 75 0c             	push   0xc(%ebp)
 80011d4:	e8 51 f9 ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 80011d9:	83 c4 10             	add    $0x10,%esp
 80011dc:	90                   	nop
 80011dd:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80011e0:	c9                   	leave
 80011e1:	c3                   	ret

080011e2 <_ZN11QPushButtonC1EP7QWidgetPKcS3_>:
 80011e2:	55                   	push   %ebp
 80011e3:	89 e5                	mov    %esp,%ebp
 80011e5:	83 ec 08             	sub    $0x8,%esp
 80011e8:	8b 45 08             	mov    0x8(%ebp),%eax
 80011eb:	83 ec 04             	sub    $0x4,%esp
 80011ee:	ff 75 10             	push   0x10(%ebp)
 80011f1:	ff 75 0c             	push   0xc(%ebp)
 80011f4:	50                   	push   %eax
 80011f5:	e8 26 fb ff ff       	call   8000d20 <_ZN7QWidgetC1EPS_PKc>
 80011fa:	83 c4 10             	add    $0x10,%esp
 80011fd:	ba 64 50 00 08       	mov    $0x8005064,%edx
 8001202:	8b 45 08             	mov    0x8(%ebp),%eax
 8001205:	89 10                	mov    %edx,(%eax)
 8001207:	8b 45 08             	mov    0x8(%ebp),%eax
 800120a:	c7 40 34 00 00 00 00 	movl   $0x0,0x34(%eax)
 8001211:	8b 45 08             	mov    0x8(%ebp),%eax
 8001214:	c6 40 38 00          	movb   $0x0,0x38(%eax)
 8001218:	83 ec 0c             	sub    $0xc,%esp
 800121b:	ff 75 14             	push   0x14(%ebp)
 800121e:	e8 89 fa ff ff       	call   8000cac <_ZL13strdup_simplePKc>
 8001223:	83 c4 10             	add    $0x10,%esp
 8001226:	8b 55 08             	mov    0x8(%ebp),%edx
 8001229:	89 42 34             	mov    %eax,0x34(%edx)
 800122c:	8b 45 08             	mov    0x8(%ebp),%eax
 800122f:	c7 40 30 80 80 80 00 	movl   $0x808080,0x30(%eax)
 8001236:	8b 45 08             	mov    0x8(%ebp),%eax
 8001239:	c7 40 24 50 00 00 00 	movl   $0x50,0x24(%eax)
 8001240:	8b 45 08             	mov    0x8(%ebp),%eax
 8001243:	c7 40 28 1c 00 00 00 	movl   $0x1c,0x28(%eax)
 800124a:	90                   	nop
 800124b:	c9                   	leave
 800124c:	c3                   	ret
 800124d:	90                   	nop

0800124e <_ZN11QPushButtonD1Ev>:
 800124e:	55                   	push   %ebp
 800124f:	89 e5                	mov    %esp,%ebp
 8001251:	83 ec 08             	sub    $0x8,%esp
 8001254:	ba 64 50 00 08       	mov    $0x8005064,%edx
 8001259:	8b 45 08             	mov    0x8(%ebp),%eax
 800125c:	89 10                	mov    %edx,(%eax)
 800125e:	8b 45 08             	mov    0x8(%ebp),%eax
 8001261:	8b 40 34             	mov    0x34(%eax),%eax
 8001264:	85 c0                	test   %eax,%eax
 8001266:	74 1c                	je     8001284 <_ZN11QPushButtonD1Ev+0x36>
 8001268:	8b 45 08             	mov    0x8(%ebp),%eax
 800126b:	8b 40 34             	mov    0x34(%eax),%eax
 800126e:	85 c0                	test   %eax,%eax
 8001270:	74 12                	je     8001284 <_ZN11QPushButtonD1Ev+0x36>
 8001272:	8b 45 08             	mov    0x8(%ebp),%eax
 8001275:	8b 40 34             	mov    0x34(%eax),%eax
 8001278:	83 ec 0c             	sub    $0xc,%esp
 800127b:	50                   	push   %eax
 800127c:	e8 77 ee ff ff       	call   80000f8 <_ZdaPv>
 8001281:	83 c4 10             	add    $0x10,%esp
 8001284:	8b 45 08             	mov    0x8(%ebp),%eax
 8001287:	83 ec 0c             	sub    $0xc,%esp
 800128a:	50                   	push   %eax
 800128b:	e8 f2 fa ff ff       	call   8000d82 <_ZN7QWidgetD1Ev>
 8001290:	83 c4 10             	add    $0x10,%esp
 8001293:	90                   	nop
 8001294:	c9                   	leave
 8001295:	c3                   	ret

08001296 <_ZN11QPushButtonD0Ev>:
 8001296:	55                   	push   %ebp
 8001297:	89 e5                	mov    %esp,%ebp
 8001299:	83 ec 08             	sub    $0x8,%esp
 800129c:	83 ec 0c             	sub    $0xc,%esp
 800129f:	ff 75 08             	push   0x8(%ebp)
 80012a2:	e8 a7 ff ff ff       	call   800124e <_ZN11QPushButtonD1Ev>
 80012a7:	83 c4 10             	add    $0x10,%esp
 80012aa:	83 ec 08             	sub    $0x8,%esp
 80012ad:	6a 3c                	push   $0x3c
 80012af:	ff 75 08             	push   0x8(%ebp)
 80012b2:	e8 47 ee ff ff       	call   80000fe <_ZdlPvj>
 80012b7:	83 c4 10             	add    $0x10,%esp
 80012ba:	c9                   	leave
 80012bb:	c3                   	ret

080012bc <_ZN11QPushButton7setTextEPKc>:
 80012bc:	55                   	push   %ebp
 80012bd:	89 e5                	mov    %esp,%ebp
 80012bf:	83 ec 08             	sub    $0x8,%esp
 80012c2:	8b 45 08             	mov    0x8(%ebp),%eax
 80012c5:	8b 40 34             	mov    0x34(%eax),%eax
 80012c8:	85 c0                	test   %eax,%eax
 80012ca:	74 1c                	je     80012e8 <_ZN11QPushButton7setTextEPKc+0x2c>
 80012cc:	8b 45 08             	mov    0x8(%ebp),%eax
 80012cf:	8b 40 34             	mov    0x34(%eax),%eax
 80012d2:	85 c0                	test   %eax,%eax
 80012d4:	74 12                	je     80012e8 <_ZN11QPushButton7setTextEPKc+0x2c>
 80012d6:	8b 45 08             	mov    0x8(%ebp),%eax
 80012d9:	8b 40 34             	mov    0x34(%eax),%eax
 80012dc:	83 ec 0c             	sub    $0xc,%esp
 80012df:	50                   	push   %eax
 80012e0:	e8 13 ee ff ff       	call   80000f8 <_ZdaPv>
 80012e5:	83 c4 10             	add    $0x10,%esp
 80012e8:	83 ec 0c             	sub    $0xc,%esp
 80012eb:	ff 75 0c             	push   0xc(%ebp)
 80012ee:	e8 b9 f9 ff ff       	call   8000cac <_ZL13strdup_simplePKc>
 80012f3:	83 c4 10             	add    $0x10,%esp
 80012f6:	8b 55 08             	mov    0x8(%ebp),%edx
 80012f9:	89 42 34             	mov    %eax,0x34(%edx)
 80012fc:	90                   	nop
 80012fd:	c9                   	leave
 80012fe:	c3                   	ret
 80012ff:	90                   	nop

08001300 <_ZN11QPushButton10paintEventEP8QPainter>:
 8001300:	55                   	push   %ebp
 8001301:	89 e5                	mov    %esp,%ebp
 8001303:	53                   	push   %ebx
 8001304:	83 ec 34             	sub    $0x34,%esp
 8001307:	8b 45 08             	mov    0x8(%ebp),%eax
 800130a:	8b 40 1c             	mov    0x1c(%eax),%eax
 800130d:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8001310:	8b 45 08             	mov    0x8(%ebp),%eax
 8001313:	8b 40 20             	mov    0x20(%eax),%eax
 8001316:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8001319:	8b 45 08             	mov    0x8(%ebp),%eax
 800131c:	8b 40 24             	mov    0x24(%eax),%eax
 800131f:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 8001322:	8b 45 08             	mov    0x8(%ebp),%eax
 8001325:	8b 40 28             	mov    0x28(%eax),%eax
 8001328:	89 45 e0             	mov    %eax,-0x20(%ebp)
 800132b:	8b 45 08             	mov    0x8(%ebp),%eax
 800132e:	0f b6 40 38          	movzbl 0x38(%eax),%eax
 8001332:	84 c0                	test   %al,%al
 8001334:	74 07                	je     800133d <_ZN11QPushButton10paintEventEP8QPainter+0x3d>
 8001336:	b8 40 40 40 00       	mov    $0x404040,%eax
 800133b:	eb 05                	jmp    8001342 <_ZN11QPushButton10paintEventEP8QPainter+0x42>
 800133d:	b8 80 80 80 00       	mov    $0x808080,%eax
 8001342:	83 ec 08             	sub    $0x8,%esp
 8001345:	50                   	push   %eax
 8001346:	ff 75 0c             	push   0xc(%ebp)
 8001349:	e8 48 f4 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 800134e:	83 c4 10             	add    $0x10,%esp
 8001351:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8001354:	8d 58 fe             	lea    -0x2(%eax),%ebx
 8001357:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800135a:	8d 48 fe             	lea    -0x2(%eax),%ecx
 800135d:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001360:	8d 50 01             	lea    0x1(%eax),%edx
 8001363:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001366:	83 c0 01             	add    $0x1,%eax
 8001369:	83 ec 0c             	sub    $0xc,%esp
 800136c:	53                   	push   %ebx
 800136d:	51                   	push   %ecx
 800136e:	52                   	push   %edx
 800136f:	50                   	push   %eax
 8001370:	ff 75 0c             	push   0xc(%ebp)
 8001373:	e8 9a f4 ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 8001378:	83 c4 20             	add    $0x20,%esp
 800137b:	8b 45 08             	mov    0x8(%ebp),%eax
 800137e:	0f b6 40 38          	movzbl 0x38(%eax),%eax
 8001382:	84 c0                	test   %al,%al
 8001384:	74 07                	je     800138d <_ZN11QPushButton10paintEventEP8QPainter+0x8d>
 8001386:	b8 00 00 00 00       	mov    $0x0,%eax
 800138b:	eb 05                	jmp    8001392 <_ZN11QPushButton10paintEventEP8QPainter+0x92>
 800138d:	b8 c0 c0 c0 00       	mov    $0xc0c0c0,%eax
 8001392:	83 ec 08             	sub    $0x8,%esp
 8001395:	50                   	push   %eax
 8001396:	ff 75 0c             	push   0xc(%ebp)
 8001399:	e8 f8 f3 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 800139e:	83 c4 10             	add    $0x10,%esp
 80013a1:	8b 55 ec             	mov    -0x14(%ebp),%edx
 80013a4:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80013a7:	01 d0                	add    %edx,%eax
 80013a9:	83 e8 01             	sub    $0x1,%eax
 80013ac:	83 ec 0c             	sub    $0xc,%esp
 80013af:	ff 75 e8             	push   -0x18(%ebp)
 80013b2:	50                   	push   %eax
 80013b3:	ff 75 e8             	push   -0x18(%ebp)
 80013b6:	ff 75 ec             	push   -0x14(%ebp)
 80013b9:	ff 75 0c             	push   0xc(%ebp)
 80013bc:	e8 05 f6 ff ff       	call   80009c6 <_ZN8QPainter8drawLineEiiii>
 80013c1:	83 c4 20             	add    $0x20,%esp
 80013c4:	8b 55 e8             	mov    -0x18(%ebp),%edx
 80013c7:	8b 45 e0             	mov    -0x20(%ebp),%eax
 80013ca:	01 d0                	add    %edx,%eax
 80013cc:	83 e8 01             	sub    $0x1,%eax
 80013cf:	83 ec 0c             	sub    $0xc,%esp
 80013d2:	50                   	push   %eax
 80013d3:	ff 75 ec             	push   -0x14(%ebp)
 80013d6:	ff 75 e8             	push   -0x18(%ebp)
 80013d9:	ff 75 ec             	push   -0x14(%ebp)
 80013dc:	ff 75 0c             	push   0xc(%ebp)
 80013df:	e8 e2 f5 ff ff       	call   80009c6 <_ZN8QPainter8drawLineEiiii>
 80013e4:	83 c4 20             	add    $0x20,%esp
 80013e7:	8b 45 08             	mov    0x8(%ebp),%eax
 80013ea:	0f b6 40 38          	movzbl 0x38(%eax),%eax
 80013ee:	84 c0                	test   %al,%al
 80013f0:	74 07                	je     80013f9 <_ZN11QPushButton10paintEventEP8QPainter+0xf9>
 80013f2:	b8 c0 c0 c0 00       	mov    $0xc0c0c0,%eax
 80013f7:	eb 05                	jmp    80013fe <_ZN11QPushButton10paintEventEP8QPainter+0xfe>
 80013f9:	b8 00 00 00 00       	mov    $0x0,%eax
 80013fe:	83 ec 08             	sub    $0x8,%esp
 8001401:	50                   	push   %eax
 8001402:	ff 75 0c             	push   0xc(%ebp)
 8001405:	e8 8c f3 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 800140a:	83 c4 10             	add    $0x10,%esp
 800140d:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8001410:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8001413:	01 d0                	add    %edx,%eax
 8001415:	8d 48 ff             	lea    -0x1(%eax),%ecx
 8001418:	8b 55 ec             	mov    -0x14(%ebp),%edx
 800141b:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800141e:	01 d0                	add    %edx,%eax
 8001420:	8d 50 ff             	lea    -0x1(%eax),%edx
 8001423:	8b 5d e8             	mov    -0x18(%ebp),%ebx
 8001426:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8001429:	01 d8                	add    %ebx,%eax
 800142b:	83 e8 01             	sub    $0x1,%eax
 800142e:	83 ec 0c             	sub    $0xc,%esp
 8001431:	51                   	push   %ecx
 8001432:	52                   	push   %edx
 8001433:	50                   	push   %eax
 8001434:	ff 75 ec             	push   -0x14(%ebp)
 8001437:	ff 75 0c             	push   0xc(%ebp)
 800143a:	e8 87 f5 ff ff       	call   80009c6 <_ZN8QPainter8drawLineEiiii>
 800143f:	83 c4 20             	add    $0x20,%esp
 8001442:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8001445:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8001448:	01 d0                	add    %edx,%eax
 800144a:	8d 48 ff             	lea    -0x1(%eax),%ecx
 800144d:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8001450:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8001453:	01 d0                	add    %edx,%eax
 8001455:	8d 50 ff             	lea    -0x1(%eax),%edx
 8001458:	8b 5d ec             	mov    -0x14(%ebp),%ebx
 800145b:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800145e:	01 d8                	add    %ebx,%eax
 8001460:	83 e8 01             	sub    $0x1,%eax
 8001463:	83 ec 0c             	sub    $0xc,%esp
 8001466:	51                   	push   %ecx
 8001467:	52                   	push   %edx
 8001468:	ff 75 e8             	push   -0x18(%ebp)
 800146b:	50                   	push   %eax
 800146c:	ff 75 0c             	push   0xc(%ebp)
 800146f:	e8 52 f5 ff ff       	call   80009c6 <_ZN8QPainter8drawLineEiiii>
 8001474:	83 c4 20             	add    $0x20,%esp
 8001477:	8b 45 08             	mov    0x8(%ebp),%eax
 800147a:	8b 40 34             	mov    0x34(%eax),%eax
 800147d:	85 c0                	test   %eax,%eax
 800147f:	0f 84 9c 00 00 00    	je     8001521 <_ZN11QPushButton10paintEventEP8QPainter+0x221>
 8001485:	8b 45 08             	mov    0x8(%ebp),%eax
 8001488:	8b 40 34             	mov    0x34(%eax),%eax
 800148b:	89 45 f4             	mov    %eax,-0xc(%ebp)
 800148e:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8001495:	eb 04                	jmp    800149b <_ZN11QPushButton10paintEventEP8QPainter+0x19b>
 8001497:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 800149b:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800149e:	8d 50 01             	lea    0x1(%eax),%edx
 80014a1:	89 55 f4             	mov    %edx,-0xc(%ebp)
 80014a4:	0f b6 00             	movzbl (%eax),%eax
 80014a7:	84 c0                	test   %al,%al
 80014a9:	0f 95 c0             	setne  %al
 80014ac:	84 c0                	test   %al,%al
 80014ae:	75 e7                	jne    8001497 <_ZN11QPushButton10paintEventEP8QPainter+0x197>
 80014b0:	e8 0f 01 00 00       	call   80015c4 <_ZN8QPainter9charWidthEv>
 80014b5:	8b 55 f0             	mov    -0x10(%ebp),%edx
 80014b8:	0f af c2             	imul   %edx,%eax
 80014bb:	89 45 dc             	mov    %eax,-0x24(%ebp)
 80014be:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80014c1:	2b 45 dc             	sub    -0x24(%ebp),%eax
 80014c4:	89 c2                	mov    %eax,%edx
 80014c6:	c1 ea 1f             	shr    $0x1f,%edx
 80014c9:	01 d0                	add    %edx,%eax
 80014cb:	d1 f8                	sar    $1,%eax
 80014cd:	89 c2                	mov    %eax,%edx
 80014cf:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80014d2:	01 d0                	add    %edx,%eax
 80014d4:	89 45 d8             	mov    %eax,-0x28(%ebp)
 80014d7:	e8 f2 00 00 00       	call   80015ce <_ZN8QPainter10charHeightEv>
 80014dc:	89 c2                	mov    %eax,%edx
 80014de:	8b 45 e0             	mov    -0x20(%ebp),%eax
 80014e1:	29 d0                	sub    %edx,%eax
 80014e3:	89 c2                	mov    %eax,%edx
 80014e5:	c1 ea 1f             	shr    $0x1f,%edx
 80014e8:	01 d0                	add    %edx,%eax
 80014ea:	d1 f8                	sar    $1,%eax
 80014ec:	89 c2                	mov    %eax,%edx
 80014ee:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80014f1:	01 d0                	add    %edx,%eax
 80014f3:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 80014f6:	83 ec 08             	sub    $0x8,%esp
 80014f9:	68 ff ff ff 00       	push   $0xffffff
 80014fe:	ff 75 0c             	push   0xc(%ebp)
 8001501:	e8 90 f2 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8001506:	83 c4 10             	add    $0x10,%esp
 8001509:	8b 45 08             	mov    0x8(%ebp),%eax
 800150c:	8b 40 34             	mov    0x34(%eax),%eax
 800150f:	50                   	push   %eax
 8001510:	ff 75 d4             	push   -0x2c(%ebp)
 8001513:	ff 75 d8             	push   -0x28(%ebp)
 8001516:	ff 75 0c             	push   0xc(%ebp)
 8001519:	e8 0c f6 ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 800151e:	83 c4 10             	add    $0x10,%esp
 8001521:	90                   	nop
 8001522:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8001525:	c9                   	leave
 8001526:	c3                   	ret
 8001527:	90                   	nop

08001528 <_ZN11QPushButton5clickEv>:
 8001528:	55                   	push   %ebp
 8001529:	89 e5                	mov    %esp,%ebp
 800152b:	83 ec 08             	sub    $0x8,%esp
 800152e:	8b 45 08             	mov    0x8(%ebp),%eax
 8001531:	83 ec 04             	sub    $0x4,%esp
 8001534:	6a 00                	push   $0x0
 8001536:	68 53 50 00 08       	push   $0x8005053
 800153b:	50                   	push   %eax
 800153c:	e8 05 f1 ff ff       	call   8000646 <_ZN7QObject10emitSignalEPKcPv>
 8001541:	83 c4 10             	add    $0x10,%esp
 8001544:	90                   	nop
 8001545:	c9                   	leave
 8001546:	c3                   	ret
 8001547:	90                   	nop

08001548 <_ZN11QPushButton10mousePressEii>:
 8001548:	55                   	push   %ebp
 8001549:	89 e5                	mov    %esp,%ebp
 800154b:	83 ec 08             	sub    $0x8,%esp
 800154e:	8b 45 08             	mov    0x8(%ebp),%eax
 8001551:	ff 75 10             	push   0x10(%ebp)
 8001554:	ff 75 0c             	push   0xc(%ebp)
 8001557:	50                   	push   %eax
 8001558:	e8 95 fa ff ff       	call   8000ff2 <_ZNK7QWidget8containsEii>
 800155d:	83 c4 0c             	add    $0xc,%esp
 8001560:	84 c0                	test   %al,%al
 8001562:	74 23                	je     8001587 <_ZN11QPushButton10mousePressEii+0x3f>
 8001564:	8b 45 08             	mov    0x8(%ebp),%eax
 8001567:	c6 40 38 01          	movb   $0x1,0x38(%eax)
 800156b:	83 ec 0c             	sub    $0xc,%esp
 800156e:	ff 75 08             	push   0x8(%ebp)
 8001571:	e8 b2 ff ff ff       	call   8001528 <_ZN11QPushButton5clickEv>
 8001576:	83 c4 10             	add    $0x10,%esp
 8001579:	8b 45 08             	mov    0x8(%ebp),%eax
 800157c:	c6 40 38 00          	movb   $0x0,0x38(%eax)
 8001580:	b8 01 00 00 00       	mov    $0x1,%eax
 8001585:	eb 05                	jmp    800158c <_ZN11QPushButton10mousePressEii+0x44>
 8001587:	b8 00 00 00 00       	mov    $0x0,%eax
 800158c:	c9                   	leave
 800158d:	c3                   	ret

0800158e <_ZNK7QObject10firstChildEv>:
 800158e:	55                   	push   %ebp
 800158f:	89 e5                	mov    %esp,%ebp
 8001591:	8b 45 08             	mov    0x8(%ebp),%eax
 8001594:	8b 40 08             	mov    0x8(%eax),%eax
 8001597:	5d                   	pop    %ebp
 8001598:	c3                   	ret
 8001599:	90                   	nop

0800159a <_ZNK7QObject11nextSiblingEv>:
 800159a:	55                   	push   %ebp
 800159b:	89 e5                	mov    %esp,%ebp
 800159d:	8b 45 08             	mov    0x8(%ebp),%eax
 80015a0:	8b 40 0c             	mov    0xc(%eax),%eax
 80015a3:	5d                   	pop    %ebp
 80015a4:	c3                   	ret
 80015a5:	90                   	nop

080015a6 <_ZNK7QWidget9classNameEv>:
 80015a6:	55                   	push   %ebp
 80015a7:	89 e5                	mov    %esp,%ebp
 80015a9:	b8 38 50 00 08       	mov    $0x8005038,%eax
 80015ae:	5d                   	pop    %ebp
 80015af:	c3                   	ret

080015b0 <_ZNK6QLabel9classNameEv>:
 80015b0:	55                   	push   %ebp
 80015b1:	89 e5                	mov    %esp,%ebp
 80015b3:	b8 40 50 00 08       	mov    $0x8005040,%eax
 80015b8:	5d                   	pop    %ebp
 80015b9:	c3                   	ret

080015ba <_ZNK11QPushButton9classNameEv>:
 80015ba:	55                   	push   %ebp
 80015bb:	89 e5                	mov    %esp,%ebp
 80015bd:	b8 47 50 00 08       	mov    $0x8005047,%eax
 80015c2:	5d                   	pop    %ebp
 80015c3:	c3                   	ret

080015c4 <_ZN8QPainter9charWidthEv>:
 80015c4:	55                   	push   %ebp
 80015c5:	89 e5                	mov    %esp,%ebp
 80015c7:	b8 08 00 00 00       	mov    $0x8,%eax
 80015cc:	5d                   	pop    %ebp
 80015cd:	c3                   	ret

080015ce <_ZN8QPainter10charHeightEv>:
 80015ce:	55                   	push   %ebp
 80015cf:	89 e5                	mov    %esp,%ebp
 80015d1:	b8 0c 00 00 00       	mov    $0xc,%eax
 80015d6:	5d                   	pop    %ebp
 80015d7:	c3                   	ret

080015d8 <_ZL13strdup_simplePKci>:
 80015d8:	55                   	push   %ebp
 80015d9:	89 e5                	mov    %esp,%ebp
 80015db:	83 ec 18             	sub    $0x18,%esp
 80015de:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 80015e2:	74 06                	je     80015ea <_ZL13strdup_simplePKci+0x12>
 80015e4:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80015e8:	7f 07                	jg     80015f1 <_ZL13strdup_simplePKci+0x19>
 80015ea:	b8 00 00 00 00       	mov    $0x0,%eax
 80015ef:	eb 4d                	jmp    800163e <_ZL13strdup_simplePKci+0x66>
 80015f1:	8b 45 0c             	mov    0xc(%ebp),%eax
 80015f4:	83 c0 01             	add    $0x1,%eax
 80015f7:	83 ec 0c             	sub    $0xc,%esp
 80015fa:	50                   	push   %eax
 80015fb:	e8 dc ea ff ff       	call   80000dc <_Znaj>
 8001600:	83 c4 10             	add    $0x10,%esp
 8001603:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8001606:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 800160d:	eb 19                	jmp    8001628 <_ZL13strdup_simplePKci+0x50>
 800160f:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8001612:	8b 45 08             	mov    0x8(%ebp),%eax
 8001615:	01 d0                	add    %edx,%eax
 8001617:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 800161a:	8b 55 f0             	mov    -0x10(%ebp),%edx
 800161d:	01 ca                	add    %ecx,%edx
 800161f:	0f b6 00             	movzbl (%eax),%eax
 8001622:	88 02                	mov    %al,(%edx)
 8001624:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8001628:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800162b:	3b 45 0c             	cmp    0xc(%ebp),%eax
 800162e:	7c df                	jl     800160f <_ZL13strdup_simplePKci+0x37>
 8001630:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001633:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001636:	01 d0                	add    %edx,%eax
 8001638:	c6 00 00             	movb   $0x0,(%eax)
 800163b:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800163e:	c9                   	leave
 800163f:	c3                   	ret

08001640 <_ZN13QTextDocumentC1Ev>:
 8001640:	55                   	push   %ebp
 8001641:	89 e5                	mov    %esp,%ebp
 8001643:	83 ec 08             	sub    $0x8,%esp
 8001646:	8b 45 08             	mov    0x8(%ebp),%eax
 8001649:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 800164f:	8b 45 08             	mov    0x8(%ebp),%eax
 8001652:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8001659:	8b 45 08             	mov    0x8(%ebp),%eax
 800165c:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
 8001663:	83 ec 08             	sub    $0x8,%esp
 8001666:	6a 08                	push   $0x8
 8001668:	ff 75 08             	push   0x8(%ebp)
 800166b:	e8 0a 02 00 00       	call   800187a <_ZN13QTextDocument18ensureLineCapacityEi>
 8001670:	83 c4 10             	add    $0x10,%esp
 8001673:	8b 45 08             	mov    0x8(%ebp),%eax
 8001676:	c7 40 04 01 00 00 00 	movl   $0x1,0x4(%eax)
 800167d:	8b 45 08             	mov    0x8(%ebp),%eax
 8001680:	8b 00                	mov    (%eax),%eax
 8001682:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 8001688:	8b 45 08             	mov    0x8(%ebp),%eax
 800168b:	8b 00                	mov    (%eax),%eax
 800168d:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8001694:	8b 45 08             	mov    0x8(%ebp),%eax
 8001697:	8b 00                	mov    (%eax),%eax
 8001699:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
 80016a0:	83 ec 04             	sub    $0x4,%esp
 80016a3:	6a 00                	push   $0x0
 80016a5:	6a 00                	push   $0x0
 80016a7:	ff 75 08             	push   0x8(%ebp)
 80016aa:	e8 21 03 00 00       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 80016af:	83 c4 10             	add    $0x10,%esp
 80016b2:	90                   	nop
 80016b3:	c9                   	leave
 80016b4:	c3                   	ret
 80016b5:	90                   	nop

080016b6 <_ZN13QTextDocumentD1Ev>:
 80016b6:	55                   	push   %ebp
 80016b7:	89 e5                	mov    %esp,%ebp
 80016b9:	83 ec 18             	sub    $0x18,%esp
 80016bc:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 80016c3:	eb 57                	jmp    800171c <_ZN13QTextDocumentD1Ev+0x66>
 80016c5:	8b 45 08             	mov    0x8(%ebp),%eax
 80016c8:	8b 08                	mov    (%eax),%ecx
 80016ca:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80016cd:	89 d0                	mov    %edx,%eax
 80016cf:	01 c0                	add    %eax,%eax
 80016d1:	01 d0                	add    %edx,%eax
 80016d3:	c1 e0 02             	shl    $0x2,%eax
 80016d6:	01 c8                	add    %ecx,%eax
 80016d8:	8b 00                	mov    (%eax),%eax
 80016da:	85 c0                	test   %eax,%eax
 80016dc:	74 3a                	je     8001718 <_ZN13QTextDocumentD1Ev+0x62>
 80016de:	8b 45 08             	mov    0x8(%ebp),%eax
 80016e1:	8b 08                	mov    (%eax),%ecx
 80016e3:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80016e6:	89 d0                	mov    %edx,%eax
 80016e8:	01 c0                	add    %eax,%eax
 80016ea:	01 d0                	add    %edx,%eax
 80016ec:	c1 e0 02             	shl    $0x2,%eax
 80016ef:	01 c8                	add    %ecx,%eax
 80016f1:	8b 00                	mov    (%eax),%eax
 80016f3:	85 c0                	test   %eax,%eax
 80016f5:	74 21                	je     8001718 <_ZN13QTextDocumentD1Ev+0x62>
 80016f7:	8b 45 08             	mov    0x8(%ebp),%eax
 80016fa:	8b 08                	mov    (%eax),%ecx
 80016fc:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80016ff:	89 d0                	mov    %edx,%eax
 8001701:	01 c0                	add    %eax,%eax
 8001703:	01 d0                	add    %edx,%eax
 8001705:	c1 e0 02             	shl    $0x2,%eax
 8001708:	01 c8                	add    %ecx,%eax
 800170a:	8b 00                	mov    (%eax),%eax
 800170c:	83 ec 0c             	sub    $0xc,%esp
 800170f:	50                   	push   %eax
 8001710:	e8 e3 e9 ff ff       	call   80000f8 <_ZdaPv>
 8001715:	83 c4 10             	add    $0x10,%esp
 8001718:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 800171c:	8b 45 08             	mov    0x8(%ebp),%eax
 800171f:	8b 40 04             	mov    0x4(%eax),%eax
 8001722:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8001725:	7c 9e                	jl     80016c5 <_ZN13QTextDocumentD1Ev+0xf>
 8001727:	8b 45 08             	mov    0x8(%ebp),%eax
 800172a:	8b 00                	mov    (%eax),%eax
 800172c:	85 c0                	test   %eax,%eax
 800172e:	74 1a                	je     800174a <_ZN13QTextDocumentD1Ev+0x94>
 8001730:	8b 45 08             	mov    0x8(%ebp),%eax
 8001733:	8b 00                	mov    (%eax),%eax
 8001735:	85 c0                	test   %eax,%eax
 8001737:	74 11                	je     800174a <_ZN13QTextDocumentD1Ev+0x94>
 8001739:	8b 45 08             	mov    0x8(%ebp),%eax
 800173c:	8b 00                	mov    (%eax),%eax
 800173e:	83 ec 0c             	sub    $0xc,%esp
 8001741:	50                   	push   %eax
 8001742:	e8 b1 e9 ff ff       	call   80000f8 <_ZdaPv>
 8001747:	83 c4 10             	add    $0x10,%esp
 800174a:	90                   	nop
 800174b:	c9                   	leave
 800174c:	c3                   	ret
 800174d:	90                   	nop

0800174e <_ZNK13QTextDocument10lineLengthEi>:
 800174e:	55                   	push   %ebp
 800174f:	89 e5                	mov    %esp,%ebp
 8001751:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001755:	78 0b                	js     8001762 <_ZNK13QTextDocument10lineLengthEi+0x14>
 8001757:	8b 45 08             	mov    0x8(%ebp),%eax
 800175a:	8b 40 04             	mov    0x4(%eax),%eax
 800175d:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001760:	7c 07                	jl     8001769 <_ZNK13QTextDocument10lineLengthEi+0x1b>
 8001762:	b8 00 00 00 00       	mov    $0x0,%eax
 8001767:	eb 16                	jmp    800177f <_ZNK13QTextDocument10lineLengthEi+0x31>
 8001769:	8b 45 08             	mov    0x8(%ebp),%eax
 800176c:	8b 08                	mov    (%eax),%ecx
 800176e:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001771:	89 d0                	mov    %edx,%eax
 8001773:	01 c0                	add    %eax,%eax
 8001775:	01 d0                	add    %edx,%eax
 8001777:	c1 e0 02             	shl    $0x2,%eax
 800177a:	01 c8                	add    %ecx,%eax
 800177c:	8b 40 04             	mov    0x4(%eax),%eax
 800177f:	5d                   	pop    %ebp
 8001780:	c3                   	ret
 8001781:	90                   	nop

08001782 <_ZNK13QTextDocument8lineTextEi>:
 8001782:	55                   	push   %ebp
 8001783:	89 e5                	mov    %esp,%ebp
 8001785:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001789:	78 0b                	js     8001796 <_ZNK13QTextDocument8lineTextEi+0x14>
 800178b:	8b 45 08             	mov    0x8(%ebp),%eax
 800178e:	8b 40 04             	mov    0x4(%eax),%eax
 8001791:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001794:	7c 07                	jl     800179d <_ZNK13QTextDocument8lineTextEi+0x1b>
 8001796:	b8 a4 50 00 08       	mov    $0x80050a4,%eax
 800179b:	eb 35                	jmp    80017d2 <_ZNK13QTextDocument8lineTextEi+0x50>
 800179d:	8b 45 08             	mov    0x8(%ebp),%eax
 80017a0:	8b 08                	mov    (%eax),%ecx
 80017a2:	8b 55 0c             	mov    0xc(%ebp),%edx
 80017a5:	89 d0                	mov    %edx,%eax
 80017a7:	01 c0                	add    %eax,%eax
 80017a9:	01 d0                	add    %edx,%eax
 80017ab:	c1 e0 02             	shl    $0x2,%eax
 80017ae:	01 c8                	add    %ecx,%eax
 80017b0:	8b 00                	mov    (%eax),%eax
 80017b2:	85 c0                	test   %eax,%eax
 80017b4:	75 07                	jne    80017bd <_ZNK13QTextDocument8lineTextEi+0x3b>
 80017b6:	b8 a4 50 00 08       	mov    $0x80050a4,%eax
 80017bb:	eb 15                	jmp    80017d2 <_ZNK13QTextDocument8lineTextEi+0x50>
 80017bd:	8b 45 08             	mov    0x8(%ebp),%eax
 80017c0:	8b 08                	mov    (%eax),%ecx
 80017c2:	8b 55 0c             	mov    0xc(%ebp),%edx
 80017c5:	89 d0                	mov    %edx,%eax
 80017c7:	01 c0                	add    %eax,%eax
 80017c9:	01 d0                	add    %edx,%eax
 80017cb:	c1 e0 02             	shl    $0x2,%eax
 80017ce:	01 c8                	add    %ecx,%eax
 80017d0:	8b 00                	mov    (%eax),%eax
 80017d2:	5d                   	pop    %ebp
 80017d3:	c3                   	ret

080017d4 <_ZNK13QTextDocument6charAtEii>:
 80017d4:	55                   	push   %ebp
 80017d5:	89 e5                	mov    %esp,%ebp
 80017d7:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80017db:	78 0b                	js     80017e8 <_ZNK13QTextDocument6charAtEii+0x14>
 80017dd:	8b 45 08             	mov    0x8(%ebp),%eax
 80017e0:	8b 40 04             	mov    0x4(%eax),%eax
 80017e3:	39 45 0c             	cmp    %eax,0xc(%ebp)
 80017e6:	7c 07                	jl     80017ef <_ZNK13QTextDocument6charAtEii+0x1b>
 80017e8:	b8 00 00 00 00       	mov    $0x0,%eax
 80017ed:	eb 45                	jmp    8001834 <_ZNK13QTextDocument6charAtEii+0x60>
 80017ef:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 80017f3:	78 1b                	js     8001810 <_ZNK13QTextDocument6charAtEii+0x3c>
 80017f5:	8b 45 08             	mov    0x8(%ebp),%eax
 80017f8:	8b 08                	mov    (%eax),%ecx
 80017fa:	8b 55 0c             	mov    0xc(%ebp),%edx
 80017fd:	89 d0                	mov    %edx,%eax
 80017ff:	01 c0                	add    %eax,%eax
 8001801:	01 d0                	add    %edx,%eax
 8001803:	c1 e0 02             	shl    $0x2,%eax
 8001806:	01 c8                	add    %ecx,%eax
 8001808:	8b 40 04             	mov    0x4(%eax),%eax
 800180b:	39 45 10             	cmp    %eax,0x10(%ebp)
 800180e:	7c 07                	jl     8001817 <_ZNK13QTextDocument6charAtEii+0x43>
 8001810:	b8 00 00 00 00       	mov    $0x0,%eax
 8001815:	eb 1d                	jmp    8001834 <_ZNK13QTextDocument6charAtEii+0x60>
 8001817:	8b 45 08             	mov    0x8(%ebp),%eax
 800181a:	8b 08                	mov    (%eax),%ecx
 800181c:	8b 55 0c             	mov    0xc(%ebp),%edx
 800181f:	89 d0                	mov    %edx,%eax
 8001821:	01 c0                	add    %eax,%eax
 8001823:	01 d0                	add    %edx,%eax
 8001825:	c1 e0 02             	shl    $0x2,%eax
 8001828:	01 c8                	add    %ecx,%eax
 800182a:	8b 10                	mov    (%eax),%edx
 800182c:	8b 45 10             	mov    0x10(%ebp),%eax
 800182f:	01 d0                	add    %edx,%eax
 8001831:	0f b6 00             	movzbl (%eax),%eax
 8001834:	5d                   	pop    %ebp
 8001835:	c3                   	ret

08001836 <_ZNK13QTextDocument11totalLengthEv>:
 8001836:	55                   	push   %ebp
 8001837:	89 e5                	mov    %esp,%ebp
 8001839:	83 ec 10             	sub    $0x10,%esp
 800183c:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8001843:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 800184a:	eb 1d                	jmp    8001869 <_ZNK13QTextDocument11totalLengthEv+0x33>
 800184c:	8b 45 08             	mov    0x8(%ebp),%eax
 800184f:	8b 08                	mov    (%eax),%ecx
 8001851:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8001854:	89 d0                	mov    %edx,%eax
 8001856:	01 c0                	add    %eax,%eax
 8001858:	01 d0                	add    %edx,%eax
 800185a:	c1 e0 02             	shl    $0x2,%eax
 800185d:	01 c8                	add    %ecx,%eax
 800185f:	8b 40 04             	mov    0x4(%eax),%eax
 8001862:	01 45 fc             	add    %eax,-0x4(%ebp)
 8001865:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
 8001869:	8b 45 08             	mov    0x8(%ebp),%eax
 800186c:	8b 40 04             	mov    0x4(%eax),%eax
 800186f:	39 45 f8             	cmp    %eax,-0x8(%ebp)
 8001872:	7c d8                	jl     800184c <_ZNK13QTextDocument11totalLengthEv+0x16>
 8001874:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8001877:	c9                   	leave
 8001878:	c3                   	ret
 8001879:	90                   	nop

0800187a <_ZN13QTextDocument18ensureLineCapacityEi>:
 800187a:	55                   	push   %ebp
 800187b:	89 e5                	mov    %esp,%ebp
 800187d:	83 ec 18             	sub    $0x18,%esp
 8001880:	8b 45 08             	mov    0x8(%ebp),%eax
 8001883:	8b 40 08             	mov    0x8(%eax),%eax
 8001886:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001889:	0f 8e 3d 01 00 00    	jle    80019cc <_ZN13QTextDocument18ensureLineCapacityEi+0x152>
 800188f:	8b 45 08             	mov    0x8(%ebp),%eax
 8001892:	8b 40 08             	mov    0x8(%eax),%eax
 8001895:	85 c0                	test   %eax,%eax
 8001897:	74 0a                	je     80018a3 <_ZN13QTextDocument18ensureLineCapacityEi+0x29>
 8001899:	8b 45 08             	mov    0x8(%ebp),%eax
 800189c:	8b 40 08             	mov    0x8(%eax),%eax
 800189f:	01 c0                	add    %eax,%eax
 80018a1:	eb 05                	jmp    80018a8 <_ZN13QTextDocument18ensureLineCapacityEi+0x2e>
 80018a3:	b8 08 00 00 00       	mov    $0x8,%eax
 80018a8:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80018ab:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80018ae:	3b 45 0c             	cmp    0xc(%ebp),%eax
 80018b1:	7d 06                	jge    80018b9 <_ZN13QTextDocument18ensureLineCapacityEi+0x3f>
 80018b3:	8b 45 0c             	mov    0xc(%ebp),%eax
 80018b6:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80018b9:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80018bc:	81 fa aa aa aa 0a    	cmp    $0xaaaaaaa,%edx
 80018c2:	77 0b                	ja     80018cf <_ZN13QTextDocument18ensureLineCapacityEi+0x55>
 80018c4:	89 d0                	mov    %edx,%eax
 80018c6:	01 c0                	add    %eax,%eax
 80018c8:	01 d0                	add    %edx,%eax
 80018ca:	c1 e0 02             	shl    $0x2,%eax
 80018cd:	eb 05                	jmp    80018d4 <_ZN13QTextDocument18ensureLineCapacityEi+0x5a>
 80018cf:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80018d4:	83 ec 0c             	sub    $0xc,%esp
 80018d7:	50                   	push   %eax
 80018d8:	e8 ff e7 ff ff       	call   80000dc <_Znaj>
 80018dd:	83 c4 10             	add    $0x10,%esp
 80018e0:	89 45 e8             	mov    %eax,-0x18(%ebp)
 80018e3:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 80018ea:	eb 3b                	jmp    8001927 <_ZN13QTextDocument18ensureLineCapacityEi+0xad>
 80018ec:	8b 45 08             	mov    0x8(%ebp),%eax
 80018ef:	8b 08                	mov    (%eax),%ecx
 80018f1:	8b 55 f0             	mov    -0x10(%ebp),%edx
 80018f4:	89 d0                	mov    %edx,%eax
 80018f6:	01 c0                	add    %eax,%eax
 80018f8:	01 d0                	add    %edx,%eax
 80018fa:	c1 e0 02             	shl    $0x2,%eax
 80018fd:	8d 14 01             	lea    (%ecx,%eax,1),%edx
 8001900:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 8001903:	89 c8                	mov    %ecx,%eax
 8001905:	01 c0                	add    %eax,%eax
 8001907:	01 c8                	add    %ecx,%eax
 8001909:	c1 e0 02             	shl    $0x2,%eax
 800190c:	89 c1                	mov    %eax,%ecx
 800190e:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001911:	01 c8                	add    %ecx,%eax
 8001913:	8b 0a                	mov    (%edx),%ecx
 8001915:	89 08                	mov    %ecx,(%eax)
 8001917:	8b 4a 04             	mov    0x4(%edx),%ecx
 800191a:	89 48 04             	mov    %ecx,0x4(%eax)
 800191d:	8b 52 08             	mov    0x8(%edx),%edx
 8001920:	89 50 08             	mov    %edx,0x8(%eax)
 8001923:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 8001927:	8b 45 08             	mov    0x8(%ebp),%eax
 800192a:	8b 40 04             	mov    0x4(%eax),%eax
 800192d:	39 45 f0             	cmp    %eax,-0x10(%ebp)
 8001930:	7c ba                	jl     80018ec <_ZN13QTextDocument18ensureLineCapacityEi+0x72>
 8001932:	8b 45 08             	mov    0x8(%ebp),%eax
 8001935:	8b 40 04             	mov    0x4(%eax),%eax
 8001938:	89 45 ec             	mov    %eax,-0x14(%ebp)
 800193b:	eb 51                	jmp    800198e <_ZN13QTextDocument18ensureLineCapacityEi+0x114>
 800193d:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8001940:	89 d0                	mov    %edx,%eax
 8001942:	01 c0                	add    %eax,%eax
 8001944:	01 d0                	add    %edx,%eax
 8001946:	c1 e0 02             	shl    $0x2,%eax
 8001949:	89 c2                	mov    %eax,%edx
 800194b:	8b 45 e8             	mov    -0x18(%ebp),%eax
 800194e:	01 d0                	add    %edx,%eax
 8001950:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 8001956:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8001959:	89 d0                	mov    %edx,%eax
 800195b:	01 c0                	add    %eax,%eax
 800195d:	01 d0                	add    %edx,%eax
 800195f:	c1 e0 02             	shl    $0x2,%eax
 8001962:	89 c2                	mov    %eax,%edx
 8001964:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001967:	01 d0                	add    %edx,%eax
 8001969:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8001970:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8001973:	89 d0                	mov    %edx,%eax
 8001975:	01 c0                	add    %eax,%eax
 8001977:	01 d0                	add    %edx,%eax
 8001979:	c1 e0 02             	shl    $0x2,%eax
 800197c:	89 c2                	mov    %eax,%edx
 800197e:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001981:	01 d0                	add    %edx,%eax
 8001983:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
 800198a:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
 800198e:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001991:	3b 45 f4             	cmp    -0xc(%ebp),%eax
 8001994:	7c a7                	jl     800193d <_ZN13QTextDocument18ensureLineCapacityEi+0xc3>
 8001996:	8b 45 08             	mov    0x8(%ebp),%eax
 8001999:	8b 00                	mov    (%eax),%eax
 800199b:	85 c0                	test   %eax,%eax
 800199d:	74 1a                	je     80019b9 <_ZN13QTextDocument18ensureLineCapacityEi+0x13f>
 800199f:	8b 45 08             	mov    0x8(%ebp),%eax
 80019a2:	8b 00                	mov    (%eax),%eax
 80019a4:	85 c0                	test   %eax,%eax
 80019a6:	74 11                	je     80019b9 <_ZN13QTextDocument18ensureLineCapacityEi+0x13f>
 80019a8:	8b 45 08             	mov    0x8(%ebp),%eax
 80019ab:	8b 00                	mov    (%eax),%eax
 80019ad:	83 ec 0c             	sub    $0xc,%esp
 80019b0:	50                   	push   %eax
 80019b1:	e8 42 e7 ff ff       	call   80000f8 <_ZdaPv>
 80019b6:	83 c4 10             	add    $0x10,%esp
 80019b9:	8b 45 08             	mov    0x8(%ebp),%eax
 80019bc:	8b 55 e8             	mov    -0x18(%ebp),%edx
 80019bf:	89 10                	mov    %edx,(%eax)
 80019c1:	8b 45 08             	mov    0x8(%ebp),%eax
 80019c4:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80019c7:	89 50 08             	mov    %edx,0x8(%eax)
 80019ca:	eb 01                	jmp    80019cd <_ZN13QTextDocument18ensureLineCapacityEi+0x153>
 80019cc:	90                   	nop
 80019cd:	c9                   	leave
 80019ce:	c3                   	ret
 80019cf:	90                   	nop

080019d0 <_ZN13QTextDocument18ensureCharCapacityEii>:
 80019d0:	55                   	push   %ebp
 80019d1:	89 e5                	mov    %esp,%ebp
 80019d3:	83 ec 18             	sub    $0x18,%esp
 80019d6:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80019da:	0f 88 eb 00 00 00    	js     8001acb <_ZN13QTextDocument18ensureCharCapacityEii+0xfb>
 80019e0:	8b 45 08             	mov    0x8(%ebp),%eax
 80019e3:	8b 40 04             	mov    0x4(%eax),%eax
 80019e6:	39 45 0c             	cmp    %eax,0xc(%ebp)
 80019e9:	0f 8d dc 00 00 00    	jge    8001acb <_ZN13QTextDocument18ensureCharCapacityEii+0xfb>
 80019ef:	8b 45 08             	mov    0x8(%ebp),%eax
 80019f2:	8b 08                	mov    (%eax),%ecx
 80019f4:	8b 55 0c             	mov    0xc(%ebp),%edx
 80019f7:	89 d0                	mov    %edx,%eax
 80019f9:	01 c0                	add    %eax,%eax
 80019fb:	01 d0                	add    %edx,%eax
 80019fd:	c1 e0 02             	shl    $0x2,%eax
 8001a00:	01 c8                	add    %ecx,%eax
 8001a02:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8001a05:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a08:	8b 40 08             	mov    0x8(%eax),%eax
 8001a0b:	39 45 10             	cmp    %eax,0x10(%ebp)
 8001a0e:	0f 8c ba 00 00 00    	jl     8001ace <_ZN13QTextDocument18ensureCharCapacityEii+0xfe>
 8001a14:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a17:	8b 40 08             	mov    0x8(%eax),%eax
 8001a1a:	85 c0                	test   %eax,%eax
 8001a1c:	74 0a                	je     8001a28 <_ZN13QTextDocument18ensureCharCapacityEii+0x58>
 8001a1e:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a21:	8b 40 08             	mov    0x8(%eax),%eax
 8001a24:	01 c0                	add    %eax,%eax
 8001a26:	eb 05                	jmp    8001a2d <_ZN13QTextDocument18ensureCharCapacityEii+0x5d>
 8001a28:	b8 40 00 00 00       	mov    $0x40,%eax
 8001a2d:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8001a30:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001a33:	3b 45 10             	cmp    0x10(%ebp),%eax
 8001a36:	7f 09                	jg     8001a41 <_ZN13QTextDocument18ensureCharCapacityEii+0x71>
 8001a38:	8b 45 10             	mov    0x10(%ebp),%eax
 8001a3b:	83 c0 40             	add    $0x40,%eax
 8001a3e:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8001a41:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001a44:	83 c0 01             	add    $0x1,%eax
 8001a47:	83 ec 0c             	sub    $0xc,%esp
 8001a4a:	50                   	push   %eax
 8001a4b:	e8 8c e6 ff ff       	call   80000dc <_Znaj>
 8001a50:	83 c4 10             	add    $0x10,%esp
 8001a53:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8001a56:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8001a5d:	eb 1b                	jmp    8001a7a <_ZN13QTextDocument18ensureCharCapacityEii+0xaa>
 8001a5f:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a62:	8b 10                	mov    (%eax),%edx
 8001a64:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001a67:	01 d0                	add    %edx,%eax
 8001a69:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 8001a6c:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8001a6f:	01 ca                	add    %ecx,%edx
 8001a71:	0f b6 00             	movzbl (%eax),%eax
 8001a74:	88 02                	mov    %al,(%edx)
 8001a76:	83 45 f0 01          	addl   $0x1,-0x10(%ebp)
 8001a7a:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a7d:	8b 40 04             	mov    0x4(%eax),%eax
 8001a80:	39 45 f0             	cmp    %eax,-0x10(%ebp)
 8001a83:	7c da                	jl     8001a5f <_ZN13QTextDocument18ensureCharCapacityEii+0x8f>
 8001a85:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a88:	8b 40 04             	mov    0x4(%eax),%eax
 8001a8b:	89 c2                	mov    %eax,%edx
 8001a8d:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001a90:	01 d0                	add    %edx,%eax
 8001a92:	c6 00 00             	movb   $0x0,(%eax)
 8001a95:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001a98:	8b 00                	mov    (%eax),%eax
 8001a9a:	85 c0                	test   %eax,%eax
 8001a9c:	74 1a                	je     8001ab8 <_ZN13QTextDocument18ensureCharCapacityEii+0xe8>
 8001a9e:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001aa1:	8b 00                	mov    (%eax),%eax
 8001aa3:	85 c0                	test   %eax,%eax
 8001aa5:	74 11                	je     8001ab8 <_ZN13QTextDocument18ensureCharCapacityEii+0xe8>
 8001aa7:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001aaa:	8b 00                	mov    (%eax),%eax
 8001aac:	83 ec 0c             	sub    $0xc,%esp
 8001aaf:	50                   	push   %eax
 8001ab0:	e8 43 e6 ff ff       	call   80000f8 <_ZdaPv>
 8001ab5:	83 c4 10             	add    $0x10,%esp
 8001ab8:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001abb:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8001abe:	89 10                	mov    %edx,(%eax)
 8001ac0:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001ac3:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8001ac6:	89 50 08             	mov    %edx,0x8(%eax)
 8001ac9:	eb 04                	jmp    8001acf <_ZN13QTextDocument18ensureCharCapacityEii+0xff>
 8001acb:	90                   	nop
 8001acc:	eb 01                	jmp    8001acf <_ZN13QTextDocument18ensureCharCapacityEii+0xff>
 8001ace:	90                   	nop
 8001acf:	c9                   	leave
 8001ad0:	c3                   	ret
 8001ad1:	90                   	nop

08001ad2 <_ZN13QTextDocument14shiftLinesDownEi>:
 8001ad2:	55                   	push   %ebp
 8001ad3:	89 e5                	mov    %esp,%ebp
 8001ad5:	53                   	push   %ebx
 8001ad6:	83 ec 14             	sub    $0x14,%esp
 8001ad9:	8b 45 08             	mov    0x8(%ebp),%eax
 8001adc:	8b 40 04             	mov    0x4(%eax),%eax
 8001adf:	83 c0 01             	add    $0x1,%eax
 8001ae2:	83 ec 08             	sub    $0x8,%esp
 8001ae5:	50                   	push   %eax
 8001ae6:	ff 75 08             	push   0x8(%ebp)
 8001ae9:	e8 8c fd ff ff       	call   800187a <_ZN13QTextDocument18ensureLineCapacityEi>
 8001aee:	83 c4 10             	add    $0x10,%esp
 8001af1:	8b 45 08             	mov    0x8(%ebp),%eax
 8001af4:	8b 40 04             	mov    0x4(%eax),%eax
 8001af7:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8001afa:	eb 3e                	jmp    8001b3a <_ZN13QTextDocument14shiftLinesDownEi+0x68>
 8001afc:	8b 45 08             	mov    0x8(%ebp),%eax
 8001aff:	8b 08                	mov    (%eax),%ecx
 8001b01:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8001b04:	89 d0                	mov    %edx,%eax
 8001b06:	01 c0                	add    %eax,%eax
 8001b08:	01 d0                	add    %edx,%eax
 8001b0a:	c1 e0 02             	shl    $0x2,%eax
 8001b0d:	83 e8 0c             	sub    $0xc,%eax
 8001b10:	8d 14 01             	lea    (%ecx,%eax,1),%edx
 8001b13:	8b 45 08             	mov    0x8(%ebp),%eax
 8001b16:	8b 18                	mov    (%eax),%ebx
 8001b18:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8001b1b:	89 c8                	mov    %ecx,%eax
 8001b1d:	01 c0                	add    %eax,%eax
 8001b1f:	01 c8                	add    %ecx,%eax
 8001b21:	c1 e0 02             	shl    $0x2,%eax
 8001b24:	01 d8                	add    %ebx,%eax
 8001b26:	8b 0a                	mov    (%edx),%ecx
 8001b28:	89 08                	mov    %ecx,(%eax)
 8001b2a:	8b 4a 04             	mov    0x4(%edx),%ecx
 8001b2d:	89 48 04             	mov    %ecx,0x4(%eax)
 8001b30:	8b 52 08             	mov    0x8(%edx),%edx
 8001b33:	89 50 08             	mov    %edx,0x8(%eax)
 8001b36:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8001b3a:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001b3d:	3b 45 0c             	cmp    0xc(%ebp),%eax
 8001b40:	7f ba                	jg     8001afc <_ZN13QTextDocument14shiftLinesDownEi+0x2a>
 8001b42:	8b 45 08             	mov    0x8(%ebp),%eax
 8001b45:	8b 08                	mov    (%eax),%ecx
 8001b47:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001b4a:	89 d0                	mov    %edx,%eax
 8001b4c:	01 c0                	add    %eax,%eax
 8001b4e:	01 d0                	add    %edx,%eax
 8001b50:	c1 e0 02             	shl    $0x2,%eax
 8001b53:	01 c8                	add    %ecx,%eax
 8001b55:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 8001b5b:	8b 45 08             	mov    0x8(%ebp),%eax
 8001b5e:	8b 08                	mov    (%eax),%ecx
 8001b60:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001b63:	89 d0                	mov    %edx,%eax
 8001b65:	01 c0                	add    %eax,%eax
 8001b67:	01 d0                	add    %edx,%eax
 8001b69:	c1 e0 02             	shl    $0x2,%eax
 8001b6c:	01 c8                	add    %ecx,%eax
 8001b6e:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8001b75:	8b 45 08             	mov    0x8(%ebp),%eax
 8001b78:	8b 08                	mov    (%eax),%ecx
 8001b7a:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001b7d:	89 d0                	mov    %edx,%eax
 8001b7f:	01 c0                	add    %eax,%eax
 8001b81:	01 d0                	add    %edx,%eax
 8001b83:	c1 e0 02             	shl    $0x2,%eax
 8001b86:	01 c8                	add    %ecx,%eax
 8001b88:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
 8001b8f:	8b 45 08             	mov    0x8(%ebp),%eax
 8001b92:	8b 40 04             	mov    0x4(%eax),%eax
 8001b95:	8d 50 01             	lea    0x1(%eax),%edx
 8001b98:	8b 45 08             	mov    0x8(%ebp),%eax
 8001b9b:	89 50 04             	mov    %edx,0x4(%eax)
 8001b9e:	90                   	nop
 8001b9f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8001ba2:	c9                   	leave
 8001ba3:	c3                   	ret

08001ba4 <_ZN13QTextDocument12shiftLinesUpEi>:
 8001ba4:	55                   	push   %ebp
 8001ba5:	89 e5                	mov    %esp,%ebp
 8001ba7:	53                   	push   %ebx
 8001ba8:	83 ec 14             	sub    $0x14,%esp
 8001bab:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001baf:	0f 88 ca 00 00 00    	js     8001c7f <_ZN13QTextDocument12shiftLinesUpEi+0xdb>
 8001bb5:	8b 45 08             	mov    0x8(%ebp),%eax
 8001bb8:	8b 40 04             	mov    0x4(%eax),%eax
 8001bbb:	83 e8 01             	sub    $0x1,%eax
 8001bbe:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001bc1:	0f 8d b8 00 00 00    	jge    8001c7f <_ZN13QTextDocument12shiftLinesUpEi+0xdb>
 8001bc7:	8b 45 08             	mov    0x8(%ebp),%eax
 8001bca:	8b 08                	mov    (%eax),%ecx
 8001bcc:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001bcf:	89 d0                	mov    %edx,%eax
 8001bd1:	01 c0                	add    %eax,%eax
 8001bd3:	01 d0                	add    %edx,%eax
 8001bd5:	c1 e0 02             	shl    $0x2,%eax
 8001bd8:	01 c8                	add    %ecx,%eax
 8001bda:	8b 00                	mov    (%eax),%eax
 8001bdc:	85 c0                	test   %eax,%eax
 8001bde:	74 3a                	je     8001c1a <_ZN13QTextDocument12shiftLinesUpEi+0x76>
 8001be0:	8b 45 08             	mov    0x8(%ebp),%eax
 8001be3:	8b 08                	mov    (%eax),%ecx
 8001be5:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001be8:	89 d0                	mov    %edx,%eax
 8001bea:	01 c0                	add    %eax,%eax
 8001bec:	01 d0                	add    %edx,%eax
 8001bee:	c1 e0 02             	shl    $0x2,%eax
 8001bf1:	01 c8                	add    %ecx,%eax
 8001bf3:	8b 00                	mov    (%eax),%eax
 8001bf5:	85 c0                	test   %eax,%eax
 8001bf7:	74 21                	je     8001c1a <_ZN13QTextDocument12shiftLinesUpEi+0x76>
 8001bf9:	8b 45 08             	mov    0x8(%ebp),%eax
 8001bfc:	8b 08                	mov    (%eax),%ecx
 8001bfe:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001c01:	89 d0                	mov    %edx,%eax
 8001c03:	01 c0                	add    %eax,%eax
 8001c05:	01 d0                	add    %edx,%eax
 8001c07:	c1 e0 02             	shl    $0x2,%eax
 8001c0a:	01 c8                	add    %ecx,%eax
 8001c0c:	8b 00                	mov    (%eax),%eax
 8001c0e:	83 ec 0c             	sub    $0xc,%esp
 8001c11:	50                   	push   %eax
 8001c12:	e8 e1 e4 ff ff       	call   80000f8 <_ZdaPv>
 8001c17:	83 c4 10             	add    $0x10,%esp
 8001c1a:	8b 45 0c             	mov    0xc(%ebp),%eax
 8001c1d:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8001c20:	eb 3e                	jmp    8001c60 <_ZN13QTextDocument12shiftLinesUpEi+0xbc>
 8001c22:	8b 45 08             	mov    0x8(%ebp),%eax
 8001c25:	8b 08                	mov    (%eax),%ecx
 8001c27:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001c2a:	8d 50 01             	lea    0x1(%eax),%edx
 8001c2d:	89 d0                	mov    %edx,%eax
 8001c2f:	01 c0                	add    %eax,%eax
 8001c31:	01 d0                	add    %edx,%eax
 8001c33:	c1 e0 02             	shl    $0x2,%eax
 8001c36:	8d 14 01             	lea    (%ecx,%eax,1),%edx
 8001c39:	8b 45 08             	mov    0x8(%ebp),%eax
 8001c3c:	8b 18                	mov    (%eax),%ebx
 8001c3e:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8001c41:	89 c8                	mov    %ecx,%eax
 8001c43:	01 c0                	add    %eax,%eax
 8001c45:	01 c8                	add    %ecx,%eax
 8001c47:	c1 e0 02             	shl    $0x2,%eax
 8001c4a:	01 d8                	add    %ebx,%eax
 8001c4c:	8b 0a                	mov    (%edx),%ecx
 8001c4e:	89 08                	mov    %ecx,(%eax)
 8001c50:	8b 4a 04             	mov    0x4(%edx),%ecx
 8001c53:	89 48 04             	mov    %ecx,0x4(%eax)
 8001c56:	8b 52 08             	mov    0x8(%edx),%edx
 8001c59:	89 50 08             	mov    %edx,0x8(%eax)
 8001c5c:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8001c60:	8b 45 08             	mov    0x8(%ebp),%eax
 8001c63:	8b 40 04             	mov    0x4(%eax),%eax
 8001c66:	83 e8 01             	sub    $0x1,%eax
 8001c69:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8001c6c:	7c b4                	jl     8001c22 <_ZN13QTextDocument12shiftLinesUpEi+0x7e>
 8001c6e:	8b 45 08             	mov    0x8(%ebp),%eax
 8001c71:	8b 40 04             	mov    0x4(%eax),%eax
 8001c74:	8d 50 ff             	lea    -0x1(%eax),%edx
 8001c77:	8b 45 08             	mov    0x8(%ebp),%eax
 8001c7a:	89 50 04             	mov    %edx,0x4(%eax)
 8001c7d:	eb 01                	jmp    8001c80 <_ZN13QTextDocument12shiftLinesUpEi+0xdc>
 8001c7f:	90                   	nop
 8001c80:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8001c83:	c9                   	leave
 8001c84:	c3                   	ret
 8001c85:	90                   	nop

08001c86 <_ZN13QTextDocument10insertCharEiic>:
 8001c86:	55                   	push   %ebp
 8001c87:	89 e5                	mov    %esp,%ebp
 8001c89:	83 ec 28             	sub    $0x28,%esp
 8001c8c:	8b 45 14             	mov    0x14(%ebp),%eax
 8001c8f:	88 45 e4             	mov    %al,-0x1c(%ebp)
 8001c92:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001c96:	0f 88 14 01 00 00    	js     8001db0 <_ZN13QTextDocument10insertCharEiic+0x12a>
 8001c9c:	8b 45 08             	mov    0x8(%ebp),%eax
 8001c9f:	8b 40 04             	mov    0x4(%eax),%eax
 8001ca2:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001ca5:	0f 8d 05 01 00 00    	jge    8001db0 <_ZN13QTextDocument10insertCharEiic+0x12a>
 8001cab:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8001caf:	78 1b                	js     8001ccc <_ZN13QTextDocument10insertCharEiic+0x46>
 8001cb1:	8b 45 08             	mov    0x8(%ebp),%eax
 8001cb4:	8b 08                	mov    (%eax),%ecx
 8001cb6:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001cb9:	89 d0                	mov    %edx,%eax
 8001cbb:	01 c0                	add    %eax,%eax
 8001cbd:	01 d0                	add    %edx,%eax
 8001cbf:	c1 e0 02             	shl    $0x2,%eax
 8001cc2:	01 c8                	add    %ecx,%eax
 8001cc4:	8b 40 04             	mov    0x4(%eax),%eax
 8001cc7:	39 45 10             	cmp    %eax,0x10(%ebp)
 8001cca:	7e 19                	jle    8001ce5 <_ZN13QTextDocument10insertCharEiic+0x5f>
 8001ccc:	8b 45 08             	mov    0x8(%ebp),%eax
 8001ccf:	8b 08                	mov    (%eax),%ecx
 8001cd1:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001cd4:	89 d0                	mov    %edx,%eax
 8001cd6:	01 c0                	add    %eax,%eax
 8001cd8:	01 d0                	add    %edx,%eax
 8001cda:	c1 e0 02             	shl    $0x2,%eax
 8001cdd:	01 c8                	add    %ecx,%eax
 8001cdf:	8b 40 04             	mov    0x4(%eax),%eax
 8001ce2:	89 45 10             	mov    %eax,0x10(%ebp)
 8001ce5:	80 7d e4 0a          	cmpb   $0xa,-0x1c(%ebp)
 8001ce9:	74 06                	je     8001cf1 <_ZN13QTextDocument10insertCharEiic+0x6b>
 8001ceb:	80 7d e4 0d          	cmpb   $0xd,-0x1c(%ebp)
 8001cef:	75 19                	jne    8001d0a <_ZN13QTextDocument10insertCharEiic+0x84>
 8001cf1:	83 ec 04             	sub    $0x4,%esp
 8001cf4:	ff 75 10             	push   0x10(%ebp)
 8001cf7:	ff 75 0c             	push   0xc(%ebp)
 8001cfa:	ff 75 08             	push   0x8(%ebp)
 8001cfd:	e8 ae 02 00 00       	call   8001fb0 <_ZN13QTextDocument13insertNewlineEii>
 8001d02:	83 c4 10             	add    $0x10,%esp
 8001d05:	e9 a7 00 00 00       	jmp    8001db1 <_ZN13QTextDocument10insertCharEiic+0x12b>
 8001d0a:	8b 45 08             	mov    0x8(%ebp),%eax
 8001d0d:	8b 08                	mov    (%eax),%ecx
 8001d0f:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001d12:	89 d0                	mov    %edx,%eax
 8001d14:	01 c0                	add    %eax,%eax
 8001d16:	01 d0                	add    %edx,%eax
 8001d18:	c1 e0 02             	shl    $0x2,%eax
 8001d1b:	01 c8                	add    %ecx,%eax
 8001d1d:	8b 40 04             	mov    0x4(%eax),%eax
 8001d20:	83 c0 01             	add    $0x1,%eax
 8001d23:	83 ec 04             	sub    $0x4,%esp
 8001d26:	50                   	push   %eax
 8001d27:	ff 75 0c             	push   0xc(%ebp)
 8001d2a:	ff 75 08             	push   0x8(%ebp)
 8001d2d:	e8 9e fc ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8001d32:	83 c4 10             	add    $0x10,%esp
 8001d35:	8b 45 08             	mov    0x8(%ebp),%eax
 8001d38:	8b 08                	mov    (%eax),%ecx
 8001d3a:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001d3d:	89 d0                	mov    %edx,%eax
 8001d3f:	01 c0                	add    %eax,%eax
 8001d41:	01 d0                	add    %edx,%eax
 8001d43:	c1 e0 02             	shl    $0x2,%eax
 8001d46:	01 c8                	add    %ecx,%eax
 8001d48:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8001d4b:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001d4e:	8b 40 04             	mov    0x4(%eax),%eax
 8001d51:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8001d54:	eb 21                	jmp    8001d77 <_ZN13QTextDocument10insertCharEiic+0xf1>
 8001d56:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001d59:	8b 00                	mov    (%eax),%eax
 8001d5b:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8001d5e:	83 ea 01             	sub    $0x1,%edx
 8001d61:	8d 0c 10             	lea    (%eax,%edx,1),%ecx
 8001d64:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001d67:	8b 10                	mov    (%eax),%edx
 8001d69:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001d6c:	01 c2                	add    %eax,%edx
 8001d6e:	0f b6 01             	movzbl (%ecx),%eax
 8001d71:	88 02                	mov    %al,(%edx)
 8001d73:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8001d77:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001d7a:	3b 45 10             	cmp    0x10(%ebp),%eax
 8001d7d:	7f d7                	jg     8001d56 <_ZN13QTextDocument10insertCharEiic+0xd0>
 8001d7f:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001d82:	8b 10                	mov    (%eax),%edx
 8001d84:	8b 45 10             	mov    0x10(%ebp),%eax
 8001d87:	01 c2                	add    %eax,%edx
 8001d89:	0f b6 45 e4          	movzbl -0x1c(%ebp),%eax
 8001d8d:	88 02                	mov    %al,(%edx)
 8001d8f:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001d92:	8b 40 04             	mov    0x4(%eax),%eax
 8001d95:	8d 50 01             	lea    0x1(%eax),%edx
 8001d98:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001d9b:	89 50 04             	mov    %edx,0x4(%eax)
 8001d9e:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001da1:	8b 10                	mov    (%eax),%edx
 8001da3:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001da6:	8b 40 04             	mov    0x4(%eax),%eax
 8001da9:	01 d0                	add    %edx,%eax
 8001dab:	c6 00 00             	movb   $0x0,(%eax)
 8001dae:	eb 01                	jmp    8001db1 <_ZN13QTextDocument10insertCharEiic+0x12b>
 8001db0:	90                   	nop
 8001db1:	c9                   	leave
 8001db2:	c3                   	ret
 8001db3:	90                   	nop

08001db4 <_ZN13QTextDocument10deleteCharEii>:
 8001db4:	55                   	push   %ebp
 8001db5:	89 e5                	mov    %esp,%ebp
 8001db7:	53                   	push   %ebx
 8001db8:	83 ec 24             	sub    $0x24,%esp
 8001dbb:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8001dbf:	7e 1d                	jle    8001dde <_ZN13QTextDocument10deleteCharEii+0x2a>
 8001dc1:	8b 45 10             	mov    0x10(%ebp),%eax
 8001dc4:	83 e8 01             	sub    $0x1,%eax
 8001dc7:	83 ec 04             	sub    $0x4,%esp
 8001dca:	50                   	push   %eax
 8001dcb:	ff 75 0c             	push   0xc(%ebp)
 8001dce:	ff 75 08             	push   0x8(%ebp)
 8001dd1:	e8 00 01 00 00       	call   8001ed6 <_ZN13QTextDocument13deleteForwardEii>
 8001dd6:	83 c4 10             	add    $0x10,%esp
 8001dd9:	e9 f2 00 00 00       	jmp    8001ed0 <_ZN13QTextDocument10deleteCharEii+0x11c>
 8001dde:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001de2:	0f 8e e8 00 00 00    	jle    8001ed0 <_ZN13QTextDocument10deleteCharEii+0x11c>
 8001de8:	8b 45 08             	mov    0x8(%ebp),%eax
 8001deb:	8b 08                	mov    (%eax),%ecx
 8001ded:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001df0:	89 d0                	mov    %edx,%eax
 8001df2:	01 c0                	add    %eax,%eax
 8001df4:	01 d0                	add    %edx,%eax
 8001df6:	c1 e0 02             	shl    $0x2,%eax
 8001df9:	83 e8 0c             	sub    $0xc,%eax
 8001dfc:	01 c8                	add    %ecx,%eax
 8001dfe:	8b 40 04             	mov    0x4(%eax),%eax
 8001e01:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8001e04:	8b 45 08             	mov    0x8(%ebp),%eax
 8001e07:	8b 08                	mov    (%eax),%ecx
 8001e09:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001e0c:	89 d0                	mov    %edx,%eax
 8001e0e:	01 c0                	add    %eax,%eax
 8001e10:	01 d0                	add    %edx,%eax
 8001e12:	c1 e0 02             	shl    $0x2,%eax
 8001e15:	01 c8                	add    %ecx,%eax
 8001e17:	8b 40 04             	mov    0x4(%eax),%eax
 8001e1a:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8001e1d:	8b 55 f0             	mov    -0x10(%ebp),%edx
 8001e20:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001e23:	01 c2                	add    %eax,%edx
 8001e25:	8b 45 0c             	mov    0xc(%ebp),%eax
 8001e28:	83 e8 01             	sub    $0x1,%eax
 8001e2b:	83 ec 04             	sub    $0x4,%esp
 8001e2e:	52                   	push   %edx
 8001e2f:	50                   	push   %eax
 8001e30:	ff 75 08             	push   0x8(%ebp)
 8001e33:	e8 98 fb ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8001e38:	83 c4 10             	add    $0x10,%esp
 8001e3b:	8b 45 08             	mov    0x8(%ebp),%eax
 8001e3e:	8b 08                	mov    (%eax),%ecx
 8001e40:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001e43:	89 d0                	mov    %edx,%eax
 8001e45:	01 c0                	add    %eax,%eax
 8001e47:	01 d0                	add    %edx,%eax
 8001e49:	c1 e0 02             	shl    $0x2,%eax
 8001e4c:	83 e8 0c             	sub    $0xc,%eax
 8001e4f:	01 c8                	add    %ecx,%eax
 8001e51:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8001e54:	8b 45 08             	mov    0x8(%ebp),%eax
 8001e57:	8b 08                	mov    (%eax),%ecx
 8001e59:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001e5c:	89 d0                	mov    %edx,%eax
 8001e5e:	01 c0                	add    %eax,%eax
 8001e60:	01 d0                	add    %edx,%eax
 8001e62:	c1 e0 02             	shl    $0x2,%eax
 8001e65:	01 c8                	add    %ecx,%eax
 8001e67:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 8001e6a:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8001e71:	eb 23                	jmp    8001e96 <_ZN13QTextDocument10deleteCharEii+0xe2>
 8001e73:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8001e76:	8b 10                	mov    (%eax),%edx
 8001e78:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001e7b:	8d 0c 02             	lea    (%edx,%eax,1),%ecx
 8001e7e:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001e81:	8b 00                	mov    (%eax),%eax
 8001e83:	8b 5d f0             	mov    -0x10(%ebp),%ebx
 8001e86:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8001e89:	01 da                	add    %ebx,%edx
 8001e8b:	01 c2                	add    %eax,%edx
 8001e8d:	0f b6 01             	movzbl (%ecx),%eax
 8001e90:	88 02                	mov    %al,(%edx)
 8001e92:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8001e96:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001e99:	3b 45 ec             	cmp    -0x14(%ebp),%eax
 8001e9c:	7c d5                	jl     8001e73 <_ZN13QTextDocument10deleteCharEii+0xbf>
 8001e9e:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001ea1:	8b 50 04             	mov    0x4(%eax),%edx
 8001ea4:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8001ea7:	01 c2                	add    %eax,%edx
 8001ea9:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001eac:	89 50 04             	mov    %edx,0x4(%eax)
 8001eaf:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001eb2:	8b 10                	mov    (%eax),%edx
 8001eb4:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8001eb7:	8b 40 04             	mov    0x4(%eax),%eax
 8001eba:	01 d0                	add    %edx,%eax
 8001ebc:	c6 00 00             	movb   $0x0,(%eax)
 8001ebf:	83 ec 08             	sub    $0x8,%esp
 8001ec2:	ff 75 0c             	push   0xc(%ebp)
 8001ec5:	ff 75 08             	push   0x8(%ebp)
 8001ec8:	e8 d7 fc ff ff       	call   8001ba4 <_ZN13QTextDocument12shiftLinesUpEi>
 8001ecd:	83 c4 10             	add    $0x10,%esp
 8001ed0:	90                   	nop
 8001ed1:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8001ed4:	c9                   	leave
 8001ed5:	c3                   	ret

08001ed6 <_ZN13QTextDocument13deleteForwardEii>:
 8001ed6:	55                   	push   %ebp
 8001ed7:	89 e5                	mov    %esp,%ebp
 8001ed9:	83 ec 18             	sub    $0x18,%esp
 8001edc:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001ee0:	0f 88 c3 00 00 00    	js     8001fa9 <_ZN13QTextDocument13deleteForwardEii+0xd3>
 8001ee6:	8b 45 08             	mov    0x8(%ebp),%eax
 8001ee9:	8b 40 04             	mov    0x4(%eax),%eax
 8001eec:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001eef:	0f 8d b4 00 00 00    	jge    8001fa9 <_ZN13QTextDocument13deleteForwardEii+0xd3>
 8001ef5:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8001ef9:	78 1b                	js     8001f16 <_ZN13QTextDocument13deleteForwardEii+0x40>
 8001efb:	8b 45 08             	mov    0x8(%ebp),%eax
 8001efe:	8b 08                	mov    (%eax),%ecx
 8001f00:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001f03:	89 d0                	mov    %edx,%eax
 8001f05:	01 c0                	add    %eax,%eax
 8001f07:	01 d0                	add    %edx,%eax
 8001f09:	c1 e0 02             	shl    $0x2,%eax
 8001f0c:	01 c8                	add    %ecx,%eax
 8001f0e:	8b 40 04             	mov    0x4(%eax),%eax
 8001f11:	39 45 10             	cmp    %eax,0x10(%ebp)
 8001f14:	7c 25                	jl     8001f3b <_ZN13QTextDocument13deleteForwardEii+0x65>
 8001f16:	8b 45 08             	mov    0x8(%ebp),%eax
 8001f19:	8b 40 04             	mov    0x4(%eax),%eax
 8001f1c:	83 e8 01             	sub    $0x1,%eax
 8001f1f:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001f22:	0f 8d 84 00 00 00    	jge    8001fac <_ZN13QTextDocument13deleteForwardEii+0xd6>
 8001f28:	83 ec 08             	sub    $0x8,%esp
 8001f2b:	ff 75 0c             	push   0xc(%ebp)
 8001f2e:	ff 75 08             	push   0x8(%ebp)
 8001f31:	e8 ac 01 00 00       	call   80020e2 <_ZN13QTextDocument13deleteNewlineEi>
 8001f36:	83 c4 10             	add    $0x10,%esp
 8001f39:	eb 71                	jmp    8001fac <_ZN13QTextDocument13deleteForwardEii+0xd6>
 8001f3b:	8b 45 08             	mov    0x8(%ebp),%eax
 8001f3e:	8b 08                	mov    (%eax),%ecx
 8001f40:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001f43:	89 d0                	mov    %edx,%eax
 8001f45:	01 c0                	add    %eax,%eax
 8001f47:	01 d0                	add    %edx,%eax
 8001f49:	c1 e0 02             	shl    $0x2,%eax
 8001f4c:	01 c8                	add    %ecx,%eax
 8001f4e:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8001f51:	8b 45 10             	mov    0x10(%ebp),%eax
 8001f54:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8001f57:	eb 21                	jmp    8001f7a <_ZN13QTextDocument13deleteForwardEii+0xa4>
 8001f59:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f5c:	8b 00                	mov    (%eax),%eax
 8001f5e:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8001f61:	83 c2 01             	add    $0x1,%edx
 8001f64:	8d 0c 10             	lea    (%eax,%edx,1),%ecx
 8001f67:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f6a:	8b 10                	mov    (%eax),%edx
 8001f6c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8001f6f:	01 c2                	add    %eax,%edx
 8001f71:	0f b6 01             	movzbl (%ecx),%eax
 8001f74:	88 02                	mov    %al,(%edx)
 8001f76:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8001f7a:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f7d:	8b 40 04             	mov    0x4(%eax),%eax
 8001f80:	83 e8 01             	sub    $0x1,%eax
 8001f83:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8001f86:	7c d1                	jl     8001f59 <_ZN13QTextDocument13deleteForwardEii+0x83>
 8001f88:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f8b:	8b 40 04             	mov    0x4(%eax),%eax
 8001f8e:	8d 50 ff             	lea    -0x1(%eax),%edx
 8001f91:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f94:	89 50 04             	mov    %edx,0x4(%eax)
 8001f97:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f9a:	8b 10                	mov    (%eax),%edx
 8001f9c:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8001f9f:	8b 40 04             	mov    0x4(%eax),%eax
 8001fa2:	01 d0                	add    %edx,%eax
 8001fa4:	c6 00 00             	movb   $0x0,(%eax)
 8001fa7:	eb 04                	jmp    8001fad <_ZN13QTextDocument13deleteForwardEii+0xd7>
 8001fa9:	90                   	nop
 8001faa:	eb 01                	jmp    8001fad <_ZN13QTextDocument13deleteForwardEii+0xd7>
 8001fac:	90                   	nop
 8001fad:	c9                   	leave
 8001fae:	c3                   	ret
 8001faf:	90                   	nop

08001fb0 <_ZN13QTextDocument13insertNewlineEii>:
 8001fb0:	55                   	push   %ebp
 8001fb1:	89 e5                	mov    %esp,%ebp
 8001fb3:	83 ec 18             	sub    $0x18,%esp
 8001fb6:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8001fba:	0f 88 1f 01 00 00    	js     80020df <_ZN13QTextDocument13insertNewlineEii+0x12f>
 8001fc0:	8b 45 08             	mov    0x8(%ebp),%eax
 8001fc3:	8b 40 04             	mov    0x4(%eax),%eax
 8001fc6:	39 45 0c             	cmp    %eax,0xc(%ebp)
 8001fc9:	0f 8d 10 01 00 00    	jge    80020df <_ZN13QTextDocument13insertNewlineEii+0x12f>
 8001fcf:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8001fd3:	78 1b                	js     8001ff0 <_ZN13QTextDocument13insertNewlineEii+0x40>
 8001fd5:	8b 45 08             	mov    0x8(%ebp),%eax
 8001fd8:	8b 08                	mov    (%eax),%ecx
 8001fda:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001fdd:	89 d0                	mov    %edx,%eax
 8001fdf:	01 c0                	add    %eax,%eax
 8001fe1:	01 d0                	add    %edx,%eax
 8001fe3:	c1 e0 02             	shl    $0x2,%eax
 8001fe6:	01 c8                	add    %ecx,%eax
 8001fe8:	8b 40 04             	mov    0x4(%eax),%eax
 8001feb:	39 45 10             	cmp    %eax,0x10(%ebp)
 8001fee:	7e 19                	jle    8002009 <_ZN13QTextDocument13insertNewlineEii+0x59>
 8001ff0:	8b 45 08             	mov    0x8(%ebp),%eax
 8001ff3:	8b 08                	mov    (%eax),%ecx
 8001ff5:	8b 55 0c             	mov    0xc(%ebp),%edx
 8001ff8:	89 d0                	mov    %edx,%eax
 8001ffa:	01 c0                	add    %eax,%eax
 8001ffc:	01 d0                	add    %edx,%eax
 8001ffe:	c1 e0 02             	shl    $0x2,%eax
 8002001:	01 c8                	add    %ecx,%eax
 8002003:	8b 40 04             	mov    0x4(%eax),%eax
 8002006:	89 45 10             	mov    %eax,0x10(%ebp)
 8002009:	8b 45 0c             	mov    0xc(%ebp),%eax
 800200c:	83 c0 01             	add    $0x1,%eax
 800200f:	83 ec 08             	sub    $0x8,%esp
 8002012:	50                   	push   %eax
 8002013:	ff 75 08             	push   0x8(%ebp)
 8002016:	e8 b7 fa ff ff       	call   8001ad2 <_ZN13QTextDocument14shiftLinesDownEi>
 800201b:	83 c4 10             	add    $0x10,%esp
 800201e:	8b 45 08             	mov    0x8(%ebp),%eax
 8002021:	8b 08                	mov    (%eax),%ecx
 8002023:	8b 55 0c             	mov    0xc(%ebp),%edx
 8002026:	89 d0                	mov    %edx,%eax
 8002028:	01 c0                	add    %eax,%eax
 800202a:	01 d0                	add    %edx,%eax
 800202c:	c1 e0 02             	shl    $0x2,%eax
 800202f:	01 c8                	add    %ecx,%eax
 8002031:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8002034:	8b 45 08             	mov    0x8(%ebp),%eax
 8002037:	8b 08                	mov    (%eax),%ecx
 8002039:	8b 45 0c             	mov    0xc(%ebp),%eax
 800203c:	8d 50 01             	lea    0x1(%eax),%edx
 800203f:	89 d0                	mov    %edx,%eax
 8002041:	01 c0                	add    %eax,%eax
 8002043:	01 d0                	add    %edx,%eax
 8002045:	c1 e0 02             	shl    $0x2,%eax
 8002048:	01 c8                	add    %ecx,%eax
 800204a:	89 45 ec             	mov    %eax,-0x14(%ebp)
 800204d:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002050:	8b 40 04             	mov    0x4(%eax),%eax
 8002053:	2b 45 10             	sub    0x10(%ebp),%eax
 8002056:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8002059:	83 7d e8 00          	cmpl   $0x0,-0x18(%ebp)
 800205d:	7e 65                	jle    80020c4 <_ZN13QTextDocument13insertNewlineEii+0x114>
 800205f:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002062:	83 c0 01             	add    $0x1,%eax
 8002065:	83 ec 04             	sub    $0x4,%esp
 8002068:	ff 75 e8             	push   -0x18(%ebp)
 800206b:	50                   	push   %eax
 800206c:	ff 75 08             	push   0x8(%ebp)
 800206f:	e8 5c f9 ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8002074:	83 c4 10             	add    $0x10,%esp
 8002077:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 800207e:	eb 23                	jmp    80020a3 <_ZN13QTextDocument13insertNewlineEii+0xf3>
 8002080:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002083:	8b 00                	mov    (%eax),%eax
 8002085:	8b 4d 10             	mov    0x10(%ebp),%ecx
 8002088:	8b 55 f4             	mov    -0xc(%ebp),%edx
 800208b:	01 ca                	add    %ecx,%edx
 800208d:	8d 0c 10             	lea    (%eax,%edx,1),%ecx
 8002090:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002093:	8b 10                	mov    (%eax),%edx
 8002095:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8002098:	01 c2                	add    %eax,%edx
 800209a:	0f b6 01             	movzbl (%ecx),%eax
 800209d:	88 02                	mov    %al,(%edx)
 800209f:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 80020a3:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80020a6:	3b 45 e8             	cmp    -0x18(%ebp),%eax
 80020a9:	7c d5                	jl     8002080 <_ZN13QTextDocument13insertNewlineEii+0xd0>
 80020ab:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80020ae:	8b 55 e8             	mov    -0x18(%ebp),%edx
 80020b1:	89 50 04             	mov    %edx,0x4(%eax)
 80020b4:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80020b7:	8b 10                	mov    (%eax),%edx
 80020b9:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80020bc:	8b 40 04             	mov    0x4(%eax),%eax
 80020bf:	01 d0                	add    %edx,%eax
 80020c1:	c6 00 00             	movb   $0x0,(%eax)
 80020c4:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80020c7:	8b 55 10             	mov    0x10(%ebp),%edx
 80020ca:	89 50 04             	mov    %edx,0x4(%eax)
 80020cd:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80020d0:	8b 10                	mov    (%eax),%edx
 80020d2:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80020d5:	8b 40 04             	mov    0x4(%eax),%eax
 80020d8:	01 d0                	add    %edx,%eax
 80020da:	c6 00 00             	movb   $0x0,(%eax)
 80020dd:	eb 01                	jmp    80020e0 <_ZN13QTextDocument13insertNewlineEii+0x130>
 80020df:	90                   	nop
 80020e0:	c9                   	leave
 80020e1:	c3                   	ret

080020e2 <_ZN13QTextDocument13deleteNewlineEi>:
 80020e2:	55                   	push   %ebp
 80020e3:	89 e5                	mov    %esp,%ebp
 80020e5:	53                   	push   %ebx
 80020e6:	83 ec 14             	sub    $0x14,%esp
 80020e9:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80020ed:	0f 88 d6 00 00 00    	js     80021c9 <_ZN13QTextDocument13deleteNewlineEi+0xe7>
 80020f3:	8b 45 08             	mov    0x8(%ebp),%eax
 80020f6:	8b 40 04             	mov    0x4(%eax),%eax
 80020f9:	83 e8 01             	sub    $0x1,%eax
 80020fc:	39 45 0c             	cmp    %eax,0xc(%ebp)
 80020ff:	0f 8d c4 00 00 00    	jge    80021c9 <_ZN13QTextDocument13deleteNewlineEi+0xe7>
 8002105:	8b 45 08             	mov    0x8(%ebp),%eax
 8002108:	8b 08                	mov    (%eax),%ecx
 800210a:	8b 55 0c             	mov    0xc(%ebp),%edx
 800210d:	89 d0                	mov    %edx,%eax
 800210f:	01 c0                	add    %eax,%eax
 8002111:	01 d0                	add    %edx,%eax
 8002113:	c1 e0 02             	shl    $0x2,%eax
 8002116:	01 c8                	add    %ecx,%eax
 8002118:	89 45 f0             	mov    %eax,-0x10(%ebp)
 800211b:	8b 45 08             	mov    0x8(%ebp),%eax
 800211e:	8b 08                	mov    (%eax),%ecx
 8002120:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002123:	8d 50 01             	lea    0x1(%eax),%edx
 8002126:	89 d0                	mov    %edx,%eax
 8002128:	01 c0                	add    %eax,%eax
 800212a:	01 d0                	add    %edx,%eax
 800212c:	c1 e0 02             	shl    $0x2,%eax
 800212f:	01 c8                	add    %ecx,%eax
 8002131:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8002134:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002137:	8b 50 04             	mov    0x4(%eax),%edx
 800213a:	8b 45 ec             	mov    -0x14(%ebp),%eax
 800213d:	8b 40 04             	mov    0x4(%eax),%eax
 8002140:	01 d0                	add    %edx,%eax
 8002142:	83 ec 04             	sub    $0x4,%esp
 8002145:	50                   	push   %eax
 8002146:	ff 75 0c             	push   0xc(%ebp)
 8002149:	ff 75 08             	push   0x8(%ebp)
 800214c:	e8 7f f8 ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8002151:	83 c4 10             	add    $0x10,%esp
 8002154:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 800215b:	eb 26                	jmp    8002183 <_ZN13QTextDocument13deleteNewlineEi+0xa1>
 800215d:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002160:	8b 10                	mov    (%eax),%edx
 8002162:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8002165:	8d 0c 02             	lea    (%edx,%eax,1),%ecx
 8002168:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800216b:	8b 10                	mov    (%eax),%edx
 800216d:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002170:	8b 58 04             	mov    0x4(%eax),%ebx
 8002173:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8002176:	01 d8                	add    %ebx,%eax
 8002178:	01 c2                	add    %eax,%edx
 800217a:	0f b6 01             	movzbl (%ecx),%eax
 800217d:	88 02                	mov    %al,(%edx)
 800217f:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8002183:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002186:	8b 40 04             	mov    0x4(%eax),%eax
 8002189:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 800218c:	7c cf                	jl     800215d <_ZN13QTextDocument13deleteNewlineEi+0x7b>
 800218e:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002191:	8b 50 04             	mov    0x4(%eax),%edx
 8002194:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002197:	8b 40 04             	mov    0x4(%eax),%eax
 800219a:	01 c2                	add    %eax,%edx
 800219c:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800219f:	89 50 04             	mov    %edx,0x4(%eax)
 80021a2:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80021a5:	8b 10                	mov    (%eax),%edx
 80021a7:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80021aa:	8b 40 04             	mov    0x4(%eax),%eax
 80021ad:	01 d0                	add    %edx,%eax
 80021af:	c6 00 00             	movb   $0x0,(%eax)
 80021b2:	8b 45 0c             	mov    0xc(%ebp),%eax
 80021b5:	83 c0 01             	add    $0x1,%eax
 80021b8:	83 ec 08             	sub    $0x8,%esp
 80021bb:	50                   	push   %eax
 80021bc:	ff 75 08             	push   0x8(%ebp)
 80021bf:	e8 e0 f9 ff ff       	call   8001ba4 <_ZN13QTextDocument12shiftLinesUpEi>
 80021c4:	83 c4 10             	add    $0x10,%esp
 80021c7:	eb 01                	jmp    80021ca <_ZN13QTextDocument13deleteNewlineEi+0xe8>
 80021c9:	90                   	nop
 80021ca:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80021cd:	c9                   	leave
 80021ce:	c3                   	ret
 80021cf:	90                   	nop

080021d0 <_ZN13QTextDocument12setPlainTextEPKc>:
 80021d0:	55                   	push   %ebp
 80021d1:	89 e5                	mov    %esp,%ebp
 80021d3:	53                   	push   %ebx
 80021d4:	83 ec 24             	sub    $0x24,%esp
 80021d7:	83 ec 0c             	sub    $0xc,%esp
 80021da:	ff 75 08             	push   0x8(%ebp)
 80021dd:	e8 7c 02 00 00       	call   800245e <_ZN13QTextDocument5clearEv>
 80021e2:	83 c4 10             	add    $0x10,%esp
 80021e5:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 80021e9:	0f 84 69 02 00 00    	je     8002458 <_ZN13QTextDocument12setPlainTextEPKc+0x288>
 80021ef:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 80021f6:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 80021fd:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
 8002204:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002207:	8b 45 0c             	mov    0xc(%ebp),%eax
 800220a:	01 d0                	add    %edx,%eax
 800220c:	0f b6 00             	movzbl (%eax),%eax
 800220f:	3c 0a                	cmp    $0xa,%al
 8002211:	74 22                	je     8002235 <_ZN13QTextDocument12setPlainTextEPKc+0x65>
 8002213:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002216:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002219:	01 d0                	add    %edx,%eax
 800221b:	0f b6 00             	movzbl (%eax),%eax
 800221e:	3c 0d                	cmp    $0xd,%al
 8002220:	74 13                	je     8002235 <_ZN13QTextDocument12setPlainTextEPKc+0x65>
 8002222:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002225:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002228:	01 d0                	add    %edx,%eax
 800222a:	0f b6 00             	movzbl (%eax),%eax
 800222d:	84 c0                	test   %al,%al
 800222f:	0f 85 2d 01 00 00    	jne    8002362 <_ZN13QTextDocument12setPlainTextEPKc+0x192>
 8002235:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002238:	2b 45 f0             	sub    -0x10(%ebp),%eax
 800223b:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 800223e:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
 8002242:	7f 22                	jg     8002266 <_ZN13QTextDocument12setPlainTextEPKc+0x96>
 8002244:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002247:	8b 45 0c             	mov    0xc(%ebp),%eax
 800224a:	01 d0                	add    %edx,%eax
 800224c:	0f b6 00             	movzbl (%eax),%eax
 800224f:	3c 0a                	cmp    $0xa,%al
 8002251:	74 13                	je     8002266 <_ZN13QTextDocument12setPlainTextEPKc+0x96>
 8002253:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002256:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002259:	01 d0                	add    %edx,%eax
 800225b:	0f b6 00             	movzbl (%eax),%eax
 800225e:	3c 0d                	cmp    $0xd,%al
 8002260:	0f 85 bf 00 00 00    	jne    8002325 <_ZN13QTextDocument12setPlainTextEPKc+0x155>
 8002266:	83 ec 04             	sub    $0x4,%esp
 8002269:	ff 75 e4             	push   -0x1c(%ebp)
 800226c:	ff 75 f4             	push   -0xc(%ebp)
 800226f:	ff 75 08             	push   0x8(%ebp)
 8002272:	e8 59 f7 ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8002277:	83 c4 10             	add    $0x10,%esp
 800227a:	8b 45 08             	mov    0x8(%ebp),%eax
 800227d:	8b 08                	mov    (%eax),%ecx
 800227f:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8002282:	89 d0                	mov    %edx,%eax
 8002284:	01 c0                	add    %eax,%eax
 8002286:	01 d0                	add    %edx,%eax
 8002288:	c1 e0 02             	shl    $0x2,%eax
 800228b:	8d 14 01             	lea    (%ecx,%eax,1),%edx
 800228e:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8002291:	89 42 04             	mov    %eax,0x4(%edx)
 8002294:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
 800229b:	eb 33                	jmp    80022d0 <_ZN13QTextDocument12setPlainTextEPKc+0x100>
 800229d:	8b 55 f0             	mov    -0x10(%ebp),%edx
 80022a0:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80022a3:	01 d0                	add    %edx,%eax
 80022a5:	89 c2                	mov    %eax,%edx
 80022a7:	8b 45 0c             	mov    0xc(%ebp),%eax
 80022aa:	8d 0c 02             	lea    (%edx,%eax,1),%ecx
 80022ad:	8b 45 08             	mov    0x8(%ebp),%eax
 80022b0:	8b 18                	mov    (%eax),%ebx
 80022b2:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80022b5:	89 d0                	mov    %edx,%eax
 80022b7:	01 c0                	add    %eax,%eax
 80022b9:	01 d0                	add    %edx,%eax
 80022bb:	c1 e0 02             	shl    $0x2,%eax
 80022be:	01 d8                	add    %ebx,%eax
 80022c0:	8b 10                	mov    (%eax),%edx
 80022c2:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80022c5:	01 c2                	add    %eax,%edx
 80022c7:	0f b6 01             	movzbl (%ecx),%eax
 80022ca:	88 02                	mov    %al,(%edx)
 80022cc:	83 45 e8 01          	addl   $0x1,-0x18(%ebp)
 80022d0:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80022d3:	3b 45 e4             	cmp    -0x1c(%ebp),%eax
 80022d6:	7c c5                	jl     800229d <_ZN13QTextDocument12setPlainTextEPKc+0xcd>
 80022d8:	8b 45 08             	mov    0x8(%ebp),%eax
 80022db:	8b 08                	mov    (%eax),%ecx
 80022dd:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80022e0:	89 d0                	mov    %edx,%eax
 80022e2:	01 c0                	add    %eax,%eax
 80022e4:	01 d0                	add    %edx,%eax
 80022e6:	c1 e0 02             	shl    $0x2,%eax
 80022e9:	01 c8                	add    %ecx,%eax
 80022eb:	8b 10                	mov    (%eax),%edx
 80022ed:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80022f0:	01 d0                	add    %edx,%eax
 80022f2:	c6 00 00             	movb   $0x0,(%eax)
 80022f5:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 80022f9:	8b 45 08             	mov    0x8(%ebp),%eax
 80022fc:	8b 40 04             	mov    0x4(%eax),%eax
 80022ff:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8002302:	7c 21                	jl     8002325 <_ZN13QTextDocument12setPlainTextEPKc+0x155>
 8002304:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8002307:	83 c0 01             	add    $0x1,%eax
 800230a:	83 ec 08             	sub    $0x8,%esp
 800230d:	50                   	push   %eax
 800230e:	ff 75 08             	push   0x8(%ebp)
 8002311:	e8 64 f5 ff ff       	call   800187a <_ZN13QTextDocument18ensureLineCapacityEi>
 8002316:	83 c4 10             	add    $0x10,%esp
 8002319:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800231c:	8d 50 01             	lea    0x1(%eax),%edx
 800231f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002322:	89 50 04             	mov    %edx,0x4(%eax)
 8002325:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002328:	8b 45 0c             	mov    0xc(%ebp),%eax
 800232b:	01 d0                	add    %edx,%eax
 800232d:	0f b6 00             	movzbl (%eax),%eax
 8002330:	3c 0d                	cmp    $0xd,%al
 8002332:	75 16                	jne    800234a <_ZN13QTextDocument12setPlainTextEPKc+0x17a>
 8002334:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002337:	8d 50 01             	lea    0x1(%eax),%edx
 800233a:	8b 45 0c             	mov    0xc(%ebp),%eax
 800233d:	01 d0                	add    %edx,%eax
 800233f:	0f b6 00             	movzbl (%eax),%eax
 8002342:	3c 0a                	cmp    $0xa,%al
 8002344:	75 04                	jne    800234a <_ZN13QTextDocument12setPlainTextEPKc+0x17a>
 8002346:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
 800234a:	8b 45 ec             	mov    -0x14(%ebp),%eax
 800234d:	83 c0 01             	add    $0x1,%eax
 8002350:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8002353:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8002356:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002359:	01 d0                	add    %edx,%eax
 800235b:	0f b6 00             	movzbl (%eax),%eax
 800235e:	84 c0                	test   %al,%al
 8002360:	74 09                	je     800236b <_ZN13QTextDocument12setPlainTextEPKc+0x19b>
 8002362:	83 45 ec 01          	addl   $0x1,-0x14(%ebp)
 8002366:	e9 99 fe ff ff       	jmp    8002204 <_ZN13QTextDocument12setPlainTextEPKc+0x34>
 800236b:	90                   	nop
 800236c:	e9 8f 00 00 00       	jmp    8002400 <_ZN13QTextDocument12setPlainTextEPKc+0x230>
 8002371:	8b 45 08             	mov    0x8(%ebp),%eax
 8002374:	8b 40 04             	mov    0x4(%eax),%eax
 8002377:	8d 50 ff             	lea    -0x1(%eax),%edx
 800237a:	8b 45 08             	mov    0x8(%ebp),%eax
 800237d:	89 50 04             	mov    %edx,0x4(%eax)
 8002380:	8b 45 08             	mov    0x8(%ebp),%eax
 8002383:	8b 10                	mov    (%eax),%edx
 8002385:	8b 45 08             	mov    0x8(%ebp),%eax
 8002388:	8b 40 04             	mov    0x4(%eax),%eax
 800238b:	89 c1                	mov    %eax,%ecx
 800238d:	89 c8                	mov    %ecx,%eax
 800238f:	01 c0                	add    %eax,%eax
 8002391:	01 c8                	add    %ecx,%eax
 8002393:	c1 e0 02             	shl    $0x2,%eax
 8002396:	01 d0                	add    %edx,%eax
 8002398:	8b 00                	mov    (%eax),%eax
 800239a:	85 c0                	test   %eax,%eax
 800239c:	74 62                	je     8002400 <_ZN13QTextDocument12setPlainTextEPKc+0x230>
 800239e:	8b 45 08             	mov    0x8(%ebp),%eax
 80023a1:	8b 10                	mov    (%eax),%edx
 80023a3:	8b 45 08             	mov    0x8(%ebp),%eax
 80023a6:	8b 40 04             	mov    0x4(%eax),%eax
 80023a9:	89 c1                	mov    %eax,%ecx
 80023ab:	89 c8                	mov    %ecx,%eax
 80023ad:	01 c0                	add    %eax,%eax
 80023af:	01 c8                	add    %ecx,%eax
 80023b1:	c1 e0 02             	shl    $0x2,%eax
 80023b4:	01 d0                	add    %edx,%eax
 80023b6:	8b 00                	mov    (%eax),%eax
 80023b8:	85 c0                	test   %eax,%eax
 80023ba:	74 26                	je     80023e2 <_ZN13QTextDocument12setPlainTextEPKc+0x212>
 80023bc:	8b 45 08             	mov    0x8(%ebp),%eax
 80023bf:	8b 10                	mov    (%eax),%edx
 80023c1:	8b 45 08             	mov    0x8(%ebp),%eax
 80023c4:	8b 40 04             	mov    0x4(%eax),%eax
 80023c7:	89 c1                	mov    %eax,%ecx
 80023c9:	89 c8                	mov    %ecx,%eax
 80023cb:	01 c0                	add    %eax,%eax
 80023cd:	01 c8                	add    %ecx,%eax
 80023cf:	c1 e0 02             	shl    $0x2,%eax
 80023d2:	01 d0                	add    %edx,%eax
 80023d4:	8b 00                	mov    (%eax),%eax
 80023d6:	83 ec 0c             	sub    $0xc,%esp
 80023d9:	50                   	push   %eax
 80023da:	e8 19 dd ff ff       	call   80000f8 <_ZdaPv>
 80023df:	83 c4 10             	add    $0x10,%esp
 80023e2:	8b 45 08             	mov    0x8(%ebp),%eax
 80023e5:	8b 10                	mov    (%eax),%edx
 80023e7:	8b 45 08             	mov    0x8(%ebp),%eax
 80023ea:	8b 40 04             	mov    0x4(%eax),%eax
 80023ed:	89 c1                	mov    %eax,%ecx
 80023ef:	89 c8                	mov    %ecx,%eax
 80023f1:	01 c0                	add    %eax,%eax
 80023f3:	01 c8                	add    %ecx,%eax
 80023f5:	c1 e0 02             	shl    $0x2,%eax
 80023f8:	01 d0                	add    %edx,%eax
 80023fa:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 8002400:	8b 45 08             	mov    0x8(%ebp),%eax
 8002403:	8b 40 04             	mov    0x4(%eax),%eax
 8002406:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8002409:	0f 8c 62 ff ff ff    	jl     8002371 <_ZN13QTextDocument12setPlainTextEPKc+0x1a1>
 800240f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002412:	8b 40 04             	mov    0x4(%eax),%eax
 8002415:	85 c0                	test   %eax,%eax
 8002417:	75 40                	jne    8002459 <_ZN13QTextDocument12setPlainTextEPKc+0x289>
 8002419:	8b 45 08             	mov    0x8(%ebp),%eax
 800241c:	c7 40 04 01 00 00 00 	movl   $0x1,0x4(%eax)
 8002423:	83 ec 04             	sub    $0x4,%esp
 8002426:	6a 00                	push   $0x0
 8002428:	6a 00                	push   $0x0
 800242a:	ff 75 08             	push   0x8(%ebp)
 800242d:	e8 9e f5 ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8002432:	83 c4 10             	add    $0x10,%esp
 8002435:	8b 45 08             	mov    0x8(%ebp),%eax
 8002438:	8b 00                	mov    (%eax),%eax
 800243a:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8002441:	8b 45 08             	mov    0x8(%ebp),%eax
 8002444:	8b 00                	mov    (%eax),%eax
 8002446:	8b 00                	mov    (%eax),%eax
 8002448:	85 c0                	test   %eax,%eax
 800244a:	74 0d                	je     8002459 <_ZN13QTextDocument12setPlainTextEPKc+0x289>
 800244c:	8b 45 08             	mov    0x8(%ebp),%eax
 800244f:	8b 00                	mov    (%eax),%eax
 8002451:	8b 00                	mov    (%eax),%eax
 8002453:	c6 00 00             	movb   $0x0,(%eax)
 8002456:	eb 01                	jmp    8002459 <_ZN13QTextDocument12setPlainTextEPKc+0x289>
 8002458:	90                   	nop
 8002459:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 800245c:	c9                   	leave
 800245d:	c3                   	ret

0800245e <_ZN13QTextDocument5clearEv>:
 800245e:	55                   	push   %ebp
 800245f:	89 e5                	mov    %esp,%ebp
 8002461:	83 ec 18             	sub    $0x18,%esp
 8002464:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 800246b:	e9 8a 00 00 00       	jmp    80024fa <_ZN13QTextDocument5clearEv+0x9c>
 8002470:	8b 45 08             	mov    0x8(%ebp),%eax
 8002473:	8b 08                	mov    (%eax),%ecx
 8002475:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8002478:	89 d0                	mov    %edx,%eax
 800247a:	01 c0                	add    %eax,%eax
 800247c:	01 d0                	add    %edx,%eax
 800247e:	c1 e0 02             	shl    $0x2,%eax
 8002481:	01 c8                	add    %ecx,%eax
 8002483:	8b 00                	mov    (%eax),%eax
 8002485:	85 c0                	test   %eax,%eax
 8002487:	74 6d                	je     80024f6 <_ZN13QTextDocument5clearEv+0x98>
 8002489:	8b 45 08             	mov    0x8(%ebp),%eax
 800248c:	8b 08                	mov    (%eax),%ecx
 800248e:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8002491:	89 d0                	mov    %edx,%eax
 8002493:	01 c0                	add    %eax,%eax
 8002495:	01 d0                	add    %edx,%eax
 8002497:	c1 e0 02             	shl    $0x2,%eax
 800249a:	01 c8                	add    %ecx,%eax
 800249c:	8b 00                	mov    (%eax),%eax
 800249e:	85 c0                	test   %eax,%eax
 80024a0:	74 21                	je     80024c3 <_ZN13QTextDocument5clearEv+0x65>
 80024a2:	8b 45 08             	mov    0x8(%ebp),%eax
 80024a5:	8b 08                	mov    (%eax),%ecx
 80024a7:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80024aa:	89 d0                	mov    %edx,%eax
 80024ac:	01 c0                	add    %eax,%eax
 80024ae:	01 d0                	add    %edx,%eax
 80024b0:	c1 e0 02             	shl    $0x2,%eax
 80024b3:	01 c8                	add    %ecx,%eax
 80024b5:	8b 00                	mov    (%eax),%eax
 80024b7:	83 ec 0c             	sub    $0xc,%esp
 80024ba:	50                   	push   %eax
 80024bb:	e8 38 dc ff ff       	call   80000f8 <_ZdaPv>
 80024c0:	83 c4 10             	add    $0x10,%esp
 80024c3:	8b 45 08             	mov    0x8(%ebp),%eax
 80024c6:	8b 08                	mov    (%eax),%ecx
 80024c8:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80024cb:	89 d0                	mov    %edx,%eax
 80024cd:	01 c0                	add    %eax,%eax
 80024cf:	01 d0                	add    %edx,%eax
 80024d1:	c1 e0 02             	shl    $0x2,%eax
 80024d4:	01 c8                	add    %ecx,%eax
 80024d6:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
 80024dc:	8b 45 08             	mov    0x8(%ebp),%eax
 80024df:	8b 08                	mov    (%eax),%ecx
 80024e1:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80024e4:	89 d0                	mov    %edx,%eax
 80024e6:	01 c0                	add    %eax,%eax
 80024e8:	01 d0                	add    %edx,%eax
 80024ea:	c1 e0 02             	shl    $0x2,%eax
 80024ed:	01 c8                	add    %ecx,%eax
 80024ef:	c7 40 08 00 00 00 00 	movl   $0x0,0x8(%eax)
 80024f6:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 80024fa:	8b 45 08             	mov    0x8(%ebp),%eax
 80024fd:	8b 40 04             	mov    0x4(%eax),%eax
 8002500:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8002503:	0f 8c 67 ff ff ff    	jl     8002470 <_ZN13QTextDocument5clearEv+0x12>
 8002509:	8b 45 08             	mov    0x8(%ebp),%eax
 800250c:	c7 40 04 01 00 00 00 	movl   $0x1,0x4(%eax)
 8002513:	83 ec 04             	sub    $0x4,%esp
 8002516:	6a 00                	push   $0x0
 8002518:	6a 00                	push   $0x0
 800251a:	ff 75 08             	push   0x8(%ebp)
 800251d:	e8 ae f4 ff ff       	call   80019d0 <_ZN13QTextDocument18ensureCharCapacityEii>
 8002522:	83 c4 10             	add    $0x10,%esp
 8002525:	8b 45 08             	mov    0x8(%ebp),%eax
 8002528:	8b 00                	mov    (%eax),%eax
 800252a:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 8002531:	90                   	nop
 8002532:	c9                   	leave
 8002533:	c3                   	ret

08002534 <_ZN9QTextEditC1EP7QWidgetPKc>:
 8002534:	55                   	push   %ebp
 8002535:	89 e5                	mov    %esp,%ebp
 8002537:	53                   	push   %ebx
 8002538:	83 ec 04             	sub    $0x4,%esp
 800253b:	8b 45 08             	mov    0x8(%ebp),%eax
 800253e:	83 ec 04             	sub    $0x4,%esp
 8002541:	ff 75 10             	push   0x10(%ebp)
 8002544:	ff 75 0c             	push   0xc(%ebp)
 8002547:	50                   	push   %eax
 8002548:	e8 d3 e7 ff ff       	call   8000d20 <_ZN7QWidgetC1EPS_PKc>
 800254d:	83 c4 10             	add    $0x10,%esp
 8002550:	ba 38 53 00 08       	mov    $0x8005338,%edx
 8002555:	8b 45 08             	mov    0x8(%ebp),%eax
 8002558:	89 10                	mov    %edx,(%eax)
 800255a:	8b 45 08             	mov    0x8(%ebp),%eax
 800255d:	c7 40 38 00 00 00 00 	movl   $0x0,0x38(%eax)
 8002564:	8b 45 08             	mov    0x8(%ebp),%eax
 8002567:	c7 40 3c 00 00 00 00 	movl   $0x0,0x3c(%eax)
 800256e:	8b 45 08             	mov    0x8(%ebp),%eax
 8002571:	c7 40 40 00 00 00 00 	movl   $0x0,0x40(%eax)
 8002578:	8b 45 08             	mov    0x8(%ebp),%eax
 800257b:	c7 40 44 14 00 00 00 	movl   $0x14,0x44(%eax)
 8002582:	8b 45 08             	mov    0x8(%ebp),%eax
 8002585:	c7 40 30 ff ff ff 00 	movl   $0xffffff,0x30(%eax)
 800258c:	83 ec 0c             	sub    $0xc,%esp
 800258f:	6a 0c                	push   $0xc
 8002591:	e8 f4 da ff ff       	call   800008a <_Znwj>
 8002596:	83 c4 10             	add    $0x10,%esp
 8002599:	89 c3                	mov    %eax,%ebx
 800259b:	83 ec 0c             	sub    $0xc,%esp
 800259e:	53                   	push   %ebx
 800259f:	e8 9c f0 ff ff       	call   8001640 <_ZN13QTextDocumentC1Ev>
 80025a4:	83 c4 10             	add    $0x10,%esp
 80025a7:	8b 45 08             	mov    0x8(%ebp),%eax
 80025aa:	89 58 34             	mov    %ebx,0x34(%eax)
 80025ad:	8b 45 08             	mov    0x8(%ebp),%eax
 80025b0:	8b 40 34             	mov    0x34(%eax),%eax
 80025b3:	83 ec 08             	sub    $0x8,%esp
 80025b6:	68 e0 51 00 08       	push   $0x80051e0
 80025bb:	50                   	push   %eax
 80025bc:	e8 0f fc ff ff       	call   80021d0 <_ZN13QTextDocument12setPlainTextEPKc>
 80025c1:	83 c4 10             	add    $0x10,%esp
 80025c4:	90                   	nop
 80025c5:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80025c8:	c9                   	leave
 80025c9:	c3                   	ret

080025ca <_ZN9QTextEditD1Ev>:
 80025ca:	55                   	push   %ebp
 80025cb:	89 e5                	mov    %esp,%ebp
 80025cd:	53                   	push   %ebx
 80025ce:	83 ec 04             	sub    $0x4,%esp
 80025d1:	ba 38 53 00 08       	mov    $0x8005338,%edx
 80025d6:	8b 45 08             	mov    0x8(%ebp),%eax
 80025d9:	89 10                	mov    %edx,(%eax)
 80025db:	8b 45 08             	mov    0x8(%ebp),%eax
 80025de:	8b 58 34             	mov    0x34(%eax),%ebx
 80025e1:	85 db                	test   %ebx,%ebx
 80025e3:	74 1a                	je     80025ff <_ZN9QTextEditD1Ev+0x35>
 80025e5:	83 ec 0c             	sub    $0xc,%esp
 80025e8:	53                   	push   %ebx
 80025e9:	e8 c8 f0 ff ff       	call   80016b6 <_ZN13QTextDocumentD1Ev>
 80025ee:	83 c4 10             	add    $0x10,%esp
 80025f1:	83 ec 08             	sub    $0x8,%esp
 80025f4:	6a 0c                	push   $0xc
 80025f6:	53                   	push   %ebx
 80025f7:	e8 02 db ff ff       	call   80000fe <_ZdlPvj>
 80025fc:	83 c4 10             	add    $0x10,%esp
 80025ff:	8b 45 08             	mov    0x8(%ebp),%eax
 8002602:	83 ec 0c             	sub    $0xc,%esp
 8002605:	50                   	push   %eax
 8002606:	e8 77 e7 ff ff       	call   8000d82 <_ZN7QWidgetD1Ev>
 800260b:	83 c4 10             	add    $0x10,%esp
 800260e:	90                   	nop
 800260f:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8002612:	c9                   	leave
 8002613:	c3                   	ret

08002614 <_ZN9QTextEditD0Ev>:
 8002614:	55                   	push   %ebp
 8002615:	89 e5                	mov    %esp,%ebp
 8002617:	83 ec 08             	sub    $0x8,%esp
 800261a:	83 ec 0c             	sub    $0xc,%esp
 800261d:	ff 75 08             	push   0x8(%ebp)
 8002620:	e8 a5 ff ff ff       	call   80025ca <_ZN9QTextEditD1Ev>
 8002625:	83 c4 10             	add    $0x10,%esp
 8002628:	83 ec 08             	sub    $0x8,%esp
 800262b:	6a 48                	push   $0x48
 800262d:	ff 75 08             	push   0x8(%ebp)
 8002630:	e8 c9 da ff ff       	call   80000fe <_ZdlPvj>
 8002635:	83 c4 10             	add    $0x10,%esp
 8002638:	c9                   	leave
 8002639:	c3                   	ret

0800263a <_ZN9QTextEdit12setPlainTextEPKc>:
 800263a:	55                   	push   %ebp
 800263b:	89 e5                	mov    %esp,%ebp
 800263d:	83 ec 08             	sub    $0x8,%esp
 8002640:	8b 45 08             	mov    0x8(%ebp),%eax
 8002643:	8b 40 34             	mov    0x34(%eax),%eax
 8002646:	83 ec 08             	sub    $0x8,%esp
 8002649:	ff 75 0c             	push   0xc(%ebp)
 800264c:	50                   	push   %eax
 800264d:	e8 7e fb ff ff       	call   80021d0 <_ZN13QTextDocument12setPlainTextEPKc>
 8002652:	83 c4 10             	add    $0x10,%esp
 8002655:	8b 45 08             	mov    0x8(%ebp),%eax
 8002658:	c7 40 38 00 00 00 00 	movl   $0x0,0x38(%eax)
 800265f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002662:	c7 40 3c 00 00 00 00 	movl   $0x0,0x3c(%eax)
 8002669:	8b 45 08             	mov    0x8(%ebp),%eax
 800266c:	c7 40 40 00 00 00 00 	movl   $0x0,0x40(%eax)
 8002673:	90                   	nop
 8002674:	c9                   	leave
 8002675:	c3                   	ret

08002676 <_ZN9QTextEdit14moveCursorLeftEv>:
 8002676:	55                   	push   %ebp
 8002677:	89 e5                	mov    %esp,%ebp
 8002679:	83 ec 08             	sub    $0x8,%esp
 800267c:	8b 45 08             	mov    0x8(%ebp),%eax
 800267f:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002682:	85 c0                	test   %eax,%eax
 8002684:	7e 11                	jle    8002697 <_ZN9QTextEdit14moveCursorLeftEv+0x21>
 8002686:	8b 45 08             	mov    0x8(%ebp),%eax
 8002689:	8b 40 3c             	mov    0x3c(%eax),%eax
 800268c:	8d 50 ff             	lea    -0x1(%eax),%edx
 800268f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002692:	89 50 3c             	mov    %edx,0x3c(%eax)
 8002695:	eb 38                	jmp    80026cf <_ZN9QTextEdit14moveCursorLeftEv+0x59>
 8002697:	8b 45 08             	mov    0x8(%ebp),%eax
 800269a:	8b 40 38             	mov    0x38(%eax),%eax
 800269d:	85 c0                	test   %eax,%eax
 800269f:	7e 2e                	jle    80026cf <_ZN9QTextEdit14moveCursorLeftEv+0x59>
 80026a1:	8b 45 08             	mov    0x8(%ebp),%eax
 80026a4:	8b 40 38             	mov    0x38(%eax),%eax
 80026a7:	8d 50 ff             	lea    -0x1(%eax),%edx
 80026aa:	8b 45 08             	mov    0x8(%ebp),%eax
 80026ad:	89 50 38             	mov    %edx,0x38(%eax)
 80026b0:	8b 45 08             	mov    0x8(%ebp),%eax
 80026b3:	8b 40 34             	mov    0x34(%eax),%eax
 80026b6:	8b 55 08             	mov    0x8(%ebp),%edx
 80026b9:	8b 52 38             	mov    0x38(%edx),%edx
 80026bc:	83 ec 08             	sub    $0x8,%esp
 80026bf:	52                   	push   %edx
 80026c0:	50                   	push   %eax
 80026c1:	e8 88 f0 ff ff       	call   800174e <_ZNK13QTextDocument10lineLengthEi>
 80026c6:	83 c4 10             	add    $0x10,%esp
 80026c9:	8b 55 08             	mov    0x8(%ebp),%edx
 80026cc:	89 42 3c             	mov    %eax,0x3c(%edx)
 80026cf:	90                   	nop
 80026d0:	c9                   	leave
 80026d1:	c3                   	ret

080026d2 <_ZN9QTextEdit15moveCursorRightEv>:
 80026d2:	55                   	push   %ebp
 80026d3:	89 e5                	mov    %esp,%ebp
 80026d5:	53                   	push   %ebx
 80026d6:	83 ec 04             	sub    $0x4,%esp
 80026d9:	8b 45 08             	mov    0x8(%ebp),%eax
 80026dc:	8b 58 3c             	mov    0x3c(%eax),%ebx
 80026df:	8b 45 08             	mov    0x8(%ebp),%eax
 80026e2:	8b 40 34             	mov    0x34(%eax),%eax
 80026e5:	8b 55 08             	mov    0x8(%ebp),%edx
 80026e8:	8b 52 38             	mov    0x38(%edx),%edx
 80026eb:	83 ec 08             	sub    $0x8,%esp
 80026ee:	52                   	push   %edx
 80026ef:	50                   	push   %eax
 80026f0:	e8 59 f0 ff ff       	call   800174e <_ZNK13QTextDocument10lineLengthEi>
 80026f5:	83 c4 10             	add    $0x10,%esp
 80026f8:	39 c3                	cmp    %eax,%ebx
 80026fa:	0f 9c c0             	setl   %al
 80026fd:	84 c0                	test   %al,%al
 80026ff:	74 11                	je     8002712 <_ZN9QTextEdit15moveCursorRightEv+0x40>
 8002701:	8b 45 08             	mov    0x8(%ebp),%eax
 8002704:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002707:	8d 50 01             	lea    0x1(%eax),%edx
 800270a:	8b 45 08             	mov    0x8(%ebp),%eax
 800270d:	89 50 3c             	mov    %edx,0x3c(%eax)
 8002710:	eb 3d                	jmp    800274f <_ZN9QTextEdit15moveCursorRightEv+0x7d>
 8002712:	8b 45 08             	mov    0x8(%ebp),%eax
 8002715:	8b 58 38             	mov    0x38(%eax),%ebx
 8002718:	8b 45 08             	mov    0x8(%ebp),%eax
 800271b:	8b 40 34             	mov    0x34(%eax),%eax
 800271e:	83 ec 0c             	sub    $0xc,%esp
 8002721:	50                   	push   %eax
 8002722:	e8 8b 09 00 00       	call   80030b2 <_ZNK13QTextDocument9lineCountEv>
 8002727:	83 c4 10             	add    $0x10,%esp
 800272a:	83 e8 01             	sub    $0x1,%eax
 800272d:	39 c3                	cmp    %eax,%ebx
 800272f:	0f 9c c0             	setl   %al
 8002732:	84 c0                	test   %al,%al
 8002734:	74 19                	je     800274f <_ZN9QTextEdit15moveCursorRightEv+0x7d>
 8002736:	8b 45 08             	mov    0x8(%ebp),%eax
 8002739:	8b 40 38             	mov    0x38(%eax),%eax
 800273c:	8d 50 01             	lea    0x1(%eax),%edx
 800273f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002742:	89 50 38             	mov    %edx,0x38(%eax)
 8002745:	8b 45 08             	mov    0x8(%ebp),%eax
 8002748:	c7 40 3c 00 00 00 00 	movl   $0x0,0x3c(%eax)
 800274f:	90                   	nop
 8002750:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8002753:	c9                   	leave
 8002754:	c3                   	ret
 8002755:	90                   	nop

08002756 <_ZN9QTextEdit12moveCursorUpEv>:
 8002756:	55                   	push   %ebp
 8002757:	89 e5                	mov    %esp,%ebp
 8002759:	83 ec 18             	sub    $0x18,%esp
 800275c:	8b 45 08             	mov    0x8(%ebp),%eax
 800275f:	8b 40 38             	mov    0x38(%eax),%eax
 8002762:	85 c0                	test   %eax,%eax
 8002764:	7e 3f                	jle    80027a5 <_ZN9QTextEdit12moveCursorUpEv+0x4f>
 8002766:	8b 45 08             	mov    0x8(%ebp),%eax
 8002769:	8b 40 38             	mov    0x38(%eax),%eax
 800276c:	8d 50 ff             	lea    -0x1(%eax),%edx
 800276f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002772:	89 50 38             	mov    %edx,0x38(%eax)
 8002775:	8b 45 08             	mov    0x8(%ebp),%eax
 8002778:	8b 40 34             	mov    0x34(%eax),%eax
 800277b:	8b 55 08             	mov    0x8(%ebp),%edx
 800277e:	8b 52 38             	mov    0x38(%edx),%edx
 8002781:	83 ec 08             	sub    $0x8,%esp
 8002784:	52                   	push   %edx
 8002785:	50                   	push   %eax
 8002786:	e8 c3 ef ff ff       	call   800174e <_ZNK13QTextDocument10lineLengthEi>
 800278b:	83 c4 10             	add    $0x10,%esp
 800278e:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8002791:	8b 45 08             	mov    0x8(%ebp),%eax
 8002794:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002797:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 800279a:	7d 09                	jge    80027a5 <_ZN9QTextEdit12moveCursorUpEv+0x4f>
 800279c:	8b 45 08             	mov    0x8(%ebp),%eax
 800279f:	8b 55 f4             	mov    -0xc(%ebp),%edx
 80027a2:	89 50 3c             	mov    %edx,0x3c(%eax)
 80027a5:	90                   	nop
 80027a6:	c9                   	leave
 80027a7:	c3                   	ret

080027a8 <_ZN9QTextEdit14moveCursorDownEv>:
 80027a8:	55                   	push   %ebp
 80027a9:	89 e5                	mov    %esp,%ebp
 80027ab:	53                   	push   %ebx
 80027ac:	83 ec 14             	sub    $0x14,%esp
 80027af:	8b 45 08             	mov    0x8(%ebp),%eax
 80027b2:	8b 58 38             	mov    0x38(%eax),%ebx
 80027b5:	8b 45 08             	mov    0x8(%ebp),%eax
 80027b8:	8b 40 34             	mov    0x34(%eax),%eax
 80027bb:	83 ec 0c             	sub    $0xc,%esp
 80027be:	50                   	push   %eax
 80027bf:	e8 ee 08 00 00       	call   80030b2 <_ZNK13QTextDocument9lineCountEv>
 80027c4:	83 c4 10             	add    $0x10,%esp
 80027c7:	83 e8 01             	sub    $0x1,%eax
 80027ca:	39 c3                	cmp    %eax,%ebx
 80027cc:	0f 9c c0             	setl   %al
 80027cf:	84 c0                	test   %al,%al
 80027d1:	74 3f                	je     8002812 <_ZN9QTextEdit14moveCursorDownEv+0x6a>
 80027d3:	8b 45 08             	mov    0x8(%ebp),%eax
 80027d6:	8b 40 38             	mov    0x38(%eax),%eax
 80027d9:	8d 50 01             	lea    0x1(%eax),%edx
 80027dc:	8b 45 08             	mov    0x8(%ebp),%eax
 80027df:	89 50 38             	mov    %edx,0x38(%eax)
 80027e2:	8b 45 08             	mov    0x8(%ebp),%eax
 80027e5:	8b 40 34             	mov    0x34(%eax),%eax
 80027e8:	8b 55 08             	mov    0x8(%ebp),%edx
 80027eb:	8b 52 38             	mov    0x38(%edx),%edx
 80027ee:	83 ec 08             	sub    $0x8,%esp
 80027f1:	52                   	push   %edx
 80027f2:	50                   	push   %eax
 80027f3:	e8 56 ef ff ff       	call   800174e <_ZNK13QTextDocument10lineLengthEi>
 80027f8:	83 c4 10             	add    $0x10,%esp
 80027fb:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80027fe:	8b 45 08             	mov    0x8(%ebp),%eax
 8002801:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002804:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8002807:	7d 09                	jge    8002812 <_ZN9QTextEdit14moveCursorDownEv+0x6a>
 8002809:	8b 45 08             	mov    0x8(%ebp),%eax
 800280c:	8b 55 f4             	mov    -0xc(%ebp),%edx
 800280f:	89 50 3c             	mov    %edx,0x3c(%eax)
 8002812:	90                   	nop
 8002813:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8002816:	c9                   	leave
 8002817:	c3                   	ret

08002818 <_ZN9QTextEdit14moveCursorHomeEv>:
 8002818:	55                   	push   %ebp
 8002819:	89 e5                	mov    %esp,%ebp
 800281b:	8b 45 08             	mov    0x8(%ebp),%eax
 800281e:	c7 40 3c 00 00 00 00 	movl   $0x0,0x3c(%eax)
 8002825:	90                   	nop
 8002826:	5d                   	pop    %ebp
 8002827:	c3                   	ret

08002828 <_ZN9QTextEdit13moveCursorEndEv>:
 8002828:	55                   	push   %ebp
 8002829:	89 e5                	mov    %esp,%ebp
 800282b:	83 ec 08             	sub    $0x8,%esp
 800282e:	8b 45 08             	mov    0x8(%ebp),%eax
 8002831:	8b 40 34             	mov    0x34(%eax),%eax
 8002834:	8b 55 08             	mov    0x8(%ebp),%edx
 8002837:	8b 52 38             	mov    0x38(%edx),%edx
 800283a:	83 ec 08             	sub    $0x8,%esp
 800283d:	52                   	push   %edx
 800283e:	50                   	push   %eax
 800283f:	e8 0a ef ff ff       	call   800174e <_ZNK13QTextDocument10lineLengthEi>
 8002844:	83 c4 10             	add    $0x10,%esp
 8002847:	8b 55 08             	mov    0x8(%ebp),%edx
 800284a:	89 42 3c             	mov    %eax,0x3c(%edx)
 800284d:	90                   	nop
 800284e:	c9                   	leave
 800284f:	c3                   	ret

08002850 <_ZN9QTextEdit19ensureCursorVisibleEv>:
 8002850:	55                   	push   %ebp
 8002851:	89 e5                	mov    %esp,%ebp
 8002853:	8b 45 08             	mov    0x8(%ebp),%eax
 8002856:	8b 50 38             	mov    0x38(%eax),%edx
 8002859:	8b 45 08             	mov    0x8(%ebp),%eax
 800285c:	8b 40 40             	mov    0x40(%eax),%eax
 800285f:	39 c2                	cmp    %eax,%edx
 8002861:	7d 0c                	jge    800286f <_ZN9QTextEdit19ensureCursorVisibleEv+0x1f>
 8002863:	8b 45 08             	mov    0x8(%ebp),%eax
 8002866:	8b 50 38             	mov    0x38(%eax),%edx
 8002869:	8b 45 08             	mov    0x8(%ebp),%eax
 800286c:	89 50 40             	mov    %edx,0x40(%eax)
 800286f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002872:	8b 50 38             	mov    0x38(%eax),%edx
 8002875:	8b 45 08             	mov    0x8(%ebp),%eax
 8002878:	8b 48 40             	mov    0x40(%eax),%ecx
 800287b:	8b 45 08             	mov    0x8(%ebp),%eax
 800287e:	8b 40 44             	mov    0x44(%eax),%eax
 8002881:	01 c8                	add    %ecx,%eax
 8002883:	39 c2                	cmp    %eax,%edx
 8002885:	7c 17                	jl     800289e <_ZN9QTextEdit19ensureCursorVisibleEv+0x4e>
 8002887:	8b 45 08             	mov    0x8(%ebp),%eax
 800288a:	8b 50 38             	mov    0x38(%eax),%edx
 800288d:	8b 45 08             	mov    0x8(%ebp),%eax
 8002890:	8b 40 44             	mov    0x44(%eax),%eax
 8002893:	29 c2                	sub    %eax,%edx
 8002895:	83 c2 01             	add    $0x1,%edx
 8002898:	8b 45 08             	mov    0x8(%ebp),%eax
 800289b:	89 50 40             	mov    %edx,0x40(%eax)
 800289e:	8b 45 08             	mov    0x8(%ebp),%eax
 80028a1:	8b 40 40             	mov    0x40(%eax),%eax
 80028a4:	85 c0                	test   %eax,%eax
 80028a6:	79 0a                	jns    80028b2 <_ZN9QTextEdit19ensureCursorVisibleEv+0x62>
 80028a8:	8b 45 08             	mov    0x8(%ebp),%eax
 80028ab:	c7 40 40 00 00 00 00 	movl   $0x0,0x40(%eax)
 80028b2:	90                   	nop
 80028b3:	5d                   	pop    %ebp
 80028b4:	c3                   	ret
 80028b5:	90                   	nop

080028b6 <_ZN9QTextEdit15scancodeToAsciiEib>:
 80028b6:	55                   	push   %ebp
 80028b7:	89 e5                	mov    %esp,%ebp
 80028b9:	83 ec 04             	sub    $0x4,%esp
 80028bc:	8b 45 0c             	mov    0xc(%ebp),%eax
 80028bf:	88 45 fc             	mov    %al,-0x4(%ebp)
 80028c2:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 80028c6:	78 26                	js     80028ee <_ZN9QTextEdit15scancodeToAsciiEib+0x38>
 80028c8:	83 7d 08 7f          	cmpl   $0x7f,0x8(%ebp)
 80028cc:	7f 20                	jg     80028ee <_ZN9QTextEdit15scancodeToAsciiEib+0x38>
 80028ce:	80 7d fc 00          	cmpb   $0x0,-0x4(%ebp)
 80028d2:	74 0d                	je     80028e1 <_ZN9QTextEdit15scancodeToAsciiEib+0x2b>
 80028d4:	8b 45 08             	mov    0x8(%ebp),%eax
 80028d7:	05 60 51 00 08       	add    $0x8005160,%eax
 80028dc:	0f b6 00             	movzbl (%eax),%eax
 80028df:	eb 12                	jmp    80028f3 <_ZN9QTextEdit15scancodeToAsciiEib+0x3d>
 80028e1:	8b 45 08             	mov    0x8(%ebp),%eax
 80028e4:	05 e0 50 00 08       	add    $0x80050e0,%eax
 80028e9:	0f b6 00             	movzbl (%eax),%eax
 80028ec:	eb 05                	jmp    80028f3 <_ZN9QTextEdit15scancodeToAsciiEib+0x3d>
 80028ee:	b8 00 00 00 00       	mov    $0x0,%eax
 80028f3:	c9                   	leave
 80028f4:	c3                   	ret
 80028f5:	90                   	nop

080028f6 <_ZN9QTextEdit8keyPressEib>:
 80028f6:	55                   	push   %ebp
 80028f7:	89 e5                	mov    %esp,%ebp
 80028f9:	53                   	push   %ebx
 80028fa:	83 ec 24             	sub    $0x24,%esp
 80028fd:	8b 45 10             	mov    0x10(%ebp),%eax
 8002900:	88 45 e4             	mov    %al,-0x1c(%ebp)
 8002903:	8b 45 0c             	mov    0xc(%ebp),%eax
 8002906:	83 e8 69             	sub    $0x69,%eax
 8002909:	83 f8 0c             	cmp    $0xc,%eax
 800290c:	0f 87 fb 00 00 00    	ja     8002a0d <_ZN9QTextEdit8keyPressEib+0x117>
 8002912:	8b 04 85 f8 52 00 08 	mov    0x80052f8(,%eax,4),%eax
 8002919:	ff e0                	jmp    *%eax
 800291b:	83 ec 0c             	sub    $0xc,%esp
 800291e:	ff 75 08             	push   0x8(%ebp)
 8002921:	e8 50 fd ff ff       	call   8002676 <_ZN9QTextEdit14moveCursorLeftEv>
 8002926:	83 c4 10             	add    $0x10,%esp
 8002929:	83 ec 0c             	sub    $0xc,%esp
 800292c:	ff 75 08             	push   0x8(%ebp)
 800292f:	e8 1c ff ff ff       	call   8002850 <_ZN9QTextEdit19ensureCursorVisibleEv>
 8002934:	83 c4 10             	add    $0x10,%esp
 8002937:	b8 01 00 00 00       	mov    $0x1,%eax
 800293c:	e9 0f 02 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002941:	83 ec 0c             	sub    $0xc,%esp
 8002944:	ff 75 08             	push   0x8(%ebp)
 8002947:	e8 86 fd ff ff       	call   80026d2 <_ZN9QTextEdit15moveCursorRightEv>
 800294c:	83 c4 10             	add    $0x10,%esp
 800294f:	83 ec 0c             	sub    $0xc,%esp
 8002952:	ff 75 08             	push   0x8(%ebp)
 8002955:	e8 f6 fe ff ff       	call   8002850 <_ZN9QTextEdit19ensureCursorVisibleEv>
 800295a:	83 c4 10             	add    $0x10,%esp
 800295d:	b8 01 00 00 00       	mov    $0x1,%eax
 8002962:	e9 e9 01 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002967:	83 ec 0c             	sub    $0xc,%esp
 800296a:	ff 75 08             	push   0x8(%ebp)
 800296d:	e8 e4 fd ff ff       	call   8002756 <_ZN9QTextEdit12moveCursorUpEv>
 8002972:	83 c4 10             	add    $0x10,%esp
 8002975:	83 ec 0c             	sub    $0xc,%esp
 8002978:	ff 75 08             	push   0x8(%ebp)
 800297b:	e8 d0 fe ff ff       	call   8002850 <_ZN9QTextEdit19ensureCursorVisibleEv>
 8002980:	83 c4 10             	add    $0x10,%esp
 8002983:	b8 01 00 00 00       	mov    $0x1,%eax
 8002988:	e9 c3 01 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 800298d:	83 ec 0c             	sub    $0xc,%esp
 8002990:	ff 75 08             	push   0x8(%ebp)
 8002993:	e8 10 fe ff ff       	call   80027a8 <_ZN9QTextEdit14moveCursorDownEv>
 8002998:	83 c4 10             	add    $0x10,%esp
 800299b:	83 ec 0c             	sub    $0xc,%esp
 800299e:	ff 75 08             	push   0x8(%ebp)
 80029a1:	e8 aa fe ff ff       	call   8002850 <_ZN9QTextEdit19ensureCursorVisibleEv>
 80029a6:	83 c4 10             	add    $0x10,%esp
 80029a9:	b8 01 00 00 00       	mov    $0x1,%eax
 80029ae:	e9 9d 01 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 80029b3:	83 ec 0c             	sub    $0xc,%esp
 80029b6:	ff 75 08             	push   0x8(%ebp)
 80029b9:	e8 5a fe ff ff       	call   8002818 <_ZN9QTextEdit14moveCursorHomeEv>
 80029be:	83 c4 10             	add    $0x10,%esp
 80029c1:	b8 01 00 00 00       	mov    $0x1,%eax
 80029c6:	e9 85 01 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 80029cb:	83 ec 0c             	sub    $0xc,%esp
 80029ce:	ff 75 08             	push   0x8(%ebp)
 80029d1:	e8 52 fe ff ff       	call   8002828 <_ZN9QTextEdit13moveCursorEndEv>
 80029d6:	83 c4 10             	add    $0x10,%esp
 80029d9:	b8 01 00 00 00       	mov    $0x1,%eax
 80029de:	e9 6d 01 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 80029e3:	8b 45 08             	mov    0x8(%ebp),%eax
 80029e6:	8b 40 34             	mov    0x34(%eax),%eax
 80029e9:	8b 55 08             	mov    0x8(%ebp),%edx
 80029ec:	8b 4a 3c             	mov    0x3c(%edx),%ecx
 80029ef:	8b 55 08             	mov    0x8(%ebp),%edx
 80029f2:	8b 52 38             	mov    0x38(%edx),%edx
 80029f5:	83 ec 04             	sub    $0x4,%esp
 80029f8:	51                   	push   %ecx
 80029f9:	52                   	push   %edx
 80029fa:	50                   	push   %eax
 80029fb:	e8 d6 f4 ff ff       	call   8001ed6 <_ZN13QTextDocument13deleteForwardEii>
 8002a00:	83 c4 10             	add    $0x10,%esp
 8002a03:	b8 01 00 00 00       	mov    $0x1,%eax
 8002a08:	e9 43 01 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002a0d:	83 7d 0c 66          	cmpl   $0x66,0xc(%ebp)
 8002a11:	75 4c                	jne    8002a5f <_ZN9QTextEdit8keyPressEib+0x169>
 8002a13:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a16:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002a19:	85 c0                	test   %eax,%eax
 8002a1b:	7f 0a                	jg     8002a27 <_ZN9QTextEdit8keyPressEib+0x131>
 8002a1d:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a20:	8b 40 38             	mov    0x38(%eax),%eax
 8002a23:	85 c0                	test   %eax,%eax
 8002a25:	7e 2e                	jle    8002a55 <_ZN9QTextEdit8keyPressEib+0x15f>
 8002a27:	83 ec 0c             	sub    $0xc,%esp
 8002a2a:	ff 75 08             	push   0x8(%ebp)
 8002a2d:	e8 44 fc ff ff       	call   8002676 <_ZN9QTextEdit14moveCursorLeftEv>
 8002a32:	83 c4 10             	add    $0x10,%esp
 8002a35:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a38:	8b 40 34             	mov    0x34(%eax),%eax
 8002a3b:	8b 55 08             	mov    0x8(%ebp),%edx
 8002a3e:	8b 4a 3c             	mov    0x3c(%edx),%ecx
 8002a41:	8b 55 08             	mov    0x8(%ebp),%edx
 8002a44:	8b 52 38             	mov    0x38(%edx),%edx
 8002a47:	83 ec 04             	sub    $0x4,%esp
 8002a4a:	51                   	push   %ecx
 8002a4b:	52                   	push   %edx
 8002a4c:	50                   	push   %eax
 8002a4d:	e8 84 f4 ff ff       	call   8001ed6 <_ZN13QTextDocument13deleteForwardEii>
 8002a52:	83 c4 10             	add    $0x10,%esp
 8002a55:	b8 01 00 00 00       	mov    $0x1,%eax
 8002a5a:	e9 f1 00 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002a5f:	83 7d 0c 5a          	cmpl   $0x5a,0xc(%ebp)
 8002a63:	75 51                	jne    8002ab6 <_ZN9QTextEdit8keyPressEib+0x1c0>
 8002a65:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a68:	8b 40 34             	mov    0x34(%eax),%eax
 8002a6b:	8b 55 08             	mov    0x8(%ebp),%edx
 8002a6e:	8b 4a 3c             	mov    0x3c(%edx),%ecx
 8002a71:	8b 55 08             	mov    0x8(%ebp),%edx
 8002a74:	8b 52 38             	mov    0x38(%edx),%edx
 8002a77:	83 ec 04             	sub    $0x4,%esp
 8002a7a:	51                   	push   %ecx
 8002a7b:	52                   	push   %edx
 8002a7c:	50                   	push   %eax
 8002a7d:	e8 2e f5 ff ff       	call   8001fb0 <_ZN13QTextDocument13insertNewlineEii>
 8002a82:	83 c4 10             	add    $0x10,%esp
 8002a85:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a88:	8b 40 38             	mov    0x38(%eax),%eax
 8002a8b:	8d 50 01             	lea    0x1(%eax),%edx
 8002a8e:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a91:	89 50 38             	mov    %edx,0x38(%eax)
 8002a94:	8b 45 08             	mov    0x8(%ebp),%eax
 8002a97:	c7 40 3c 00 00 00 00 	movl   $0x0,0x3c(%eax)
 8002a9e:	83 ec 0c             	sub    $0xc,%esp
 8002aa1:	ff 75 08             	push   0x8(%ebp)
 8002aa4:	e8 a7 fd ff ff       	call   8002850 <_ZN9QTextEdit19ensureCursorVisibleEv>
 8002aa9:	83 c4 10             	add    $0x10,%esp
 8002aac:	b8 01 00 00 00       	mov    $0x1,%eax
 8002ab1:	e9 9a 00 00 00       	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002ab6:	83 7d 0c 0d          	cmpl   $0xd,0xc(%ebp)
 8002aba:	75 35                	jne    8002af1 <_ZN9QTextEdit8keyPressEib+0x1fb>
 8002abc:	8b 45 08             	mov    0x8(%ebp),%eax
 8002abf:	8b 40 34             	mov    0x34(%eax),%eax
 8002ac2:	8b 55 08             	mov    0x8(%ebp),%edx
 8002ac5:	8b 4a 3c             	mov    0x3c(%edx),%ecx
 8002ac8:	8b 55 08             	mov    0x8(%ebp),%edx
 8002acb:	8b 52 38             	mov    0x38(%edx),%edx
 8002ace:	6a 09                	push   $0x9
 8002ad0:	51                   	push   %ecx
 8002ad1:	52                   	push   %edx
 8002ad2:	50                   	push   %eax
 8002ad3:	e8 ae f1 ff ff       	call   8001c86 <_ZN13QTextDocument10insertCharEiic>
 8002ad8:	83 c4 10             	add    $0x10,%esp
 8002adb:	8b 45 08             	mov    0x8(%ebp),%eax
 8002ade:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002ae1:	8d 50 01             	lea    0x1(%eax),%edx
 8002ae4:	8b 45 08             	mov    0x8(%ebp),%eax
 8002ae7:	89 50 3c             	mov    %edx,0x3c(%eax)
 8002aea:	b8 01 00 00 00       	mov    $0x1,%eax
 8002aef:	eb 5f                	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002af1:	0f b6 45 e4          	movzbl -0x1c(%ebp),%eax
 8002af5:	83 ec 08             	sub    $0x8,%esp
 8002af8:	50                   	push   %eax
 8002af9:	ff 75 0c             	push   0xc(%ebp)
 8002afc:	e8 b5 fd ff ff       	call   80028b6 <_ZN9QTextEdit15scancodeToAsciiEib>
 8002b01:	83 c4 10             	add    $0x10,%esp
 8002b04:	88 45 f7             	mov    %al,-0x9(%ebp)
 8002b07:	80 7d f7 1f          	cmpb   $0x1f,-0x9(%ebp)
 8002b0b:	7f 06                	jg     8002b13 <_ZN9QTextEdit8keyPressEib+0x21d>
 8002b0d:	80 7d f7 09          	cmpb   $0x9,-0x9(%ebp)
 8002b11:	75 38                	jne    8002b4b <_ZN9QTextEdit8keyPressEib+0x255>
 8002b13:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b16:	8b 40 34             	mov    0x34(%eax),%eax
 8002b19:	0f be 5d f7          	movsbl -0x9(%ebp),%ebx
 8002b1d:	8b 55 08             	mov    0x8(%ebp),%edx
 8002b20:	8b 4a 3c             	mov    0x3c(%edx),%ecx
 8002b23:	8b 55 08             	mov    0x8(%ebp),%edx
 8002b26:	8b 52 38             	mov    0x38(%edx),%edx
 8002b29:	53                   	push   %ebx
 8002b2a:	51                   	push   %ecx
 8002b2b:	52                   	push   %edx
 8002b2c:	50                   	push   %eax
 8002b2d:	e8 54 f1 ff ff       	call   8001c86 <_ZN13QTextDocument10insertCharEiic>
 8002b32:	83 c4 10             	add    $0x10,%esp
 8002b35:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b38:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002b3b:	8d 50 01             	lea    0x1(%eax),%edx
 8002b3e:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b41:	89 50 3c             	mov    %edx,0x3c(%eax)
 8002b44:	b8 01 00 00 00       	mov    $0x1,%eax
 8002b49:	eb 05                	jmp    8002b50 <_ZN9QTextEdit8keyPressEib+0x25a>
 8002b4b:	b8 00 00 00 00       	mov    $0x0,%eax
 8002b50:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8002b53:	c9                   	leave
 8002b54:	c3                   	ret
 8002b55:	90                   	nop

08002b56 <_ZN9QTextEdit10paintEventEP8QPainter>:
 8002b56:	55                   	push   %ebp
 8002b57:	89 e5                	mov    %esp,%ebp
 8002b59:	53                   	push   %ebx
 8002b5a:	83 ec 74             	sub    $0x74,%esp
 8002b5d:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b60:	8b 40 1c             	mov    0x1c(%eax),%eax
 8002b63:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8002b66:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b69:	8b 40 20             	mov    0x20(%eax),%eax
 8002b6c:	89 45 dc             	mov    %eax,-0x24(%ebp)
 8002b6f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b72:	8b 40 24             	mov    0x24(%eax),%eax
 8002b75:	89 45 d8             	mov    %eax,-0x28(%ebp)
 8002b78:	8b 45 08             	mov    0x8(%ebp),%eax
 8002b7b:	8b 40 28             	mov    0x28(%eax),%eax
 8002b7e:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 8002b81:	e8 48 ea ff ff       	call   80015ce <_ZN8QPainter10charHeightEv>
 8002b86:	89 45 d0             	mov    %eax,-0x30(%ebp)
 8002b89:	e8 36 ea ff ff       	call   80015c4 <_ZN8QPainter9charWidthEv>
 8002b8e:	89 45 cc             	mov    %eax,-0x34(%ebp)
 8002b91:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 8002b94:	83 e8 04             	sub    $0x4,%eax
 8002b97:	99                   	cltd
 8002b98:	f7 7d d0             	idivl  -0x30(%ebp)
 8002b9b:	89 c2                	mov    %eax,%edx
 8002b9d:	8b 45 08             	mov    0x8(%ebp),%eax
 8002ba0:	89 50 44             	mov    %edx,0x44(%eax)
 8002ba3:	8b 45 08             	mov    0x8(%ebp),%eax
 8002ba6:	8b 40 44             	mov    0x44(%eax),%eax
 8002ba9:	85 c0                	test   %eax,%eax
 8002bab:	7f 0a                	jg     8002bb7 <_ZN9QTextEdit10paintEventEP8QPainter+0x61>
 8002bad:	8b 45 08             	mov    0x8(%ebp),%eax
 8002bb0:	c7 40 44 01 00 00 00 	movl   $0x1,0x44(%eax)
 8002bb7:	8b 45 08             	mov    0x8(%ebp),%eax
 8002bba:	8b 40 30             	mov    0x30(%eax),%eax
 8002bbd:	83 ec 08             	sub    $0x8,%esp
 8002bc0:	50                   	push   %eax
 8002bc1:	ff 75 0c             	push   0xc(%ebp)
 8002bc4:	e8 cd db ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002bc9:	83 c4 10             	add    $0x10,%esp
 8002bcc:	83 ec 0c             	sub    $0xc,%esp
 8002bcf:	ff 75 d4             	push   -0x2c(%ebp)
 8002bd2:	ff 75 d8             	push   -0x28(%ebp)
 8002bd5:	ff 75 dc             	push   -0x24(%ebp)
 8002bd8:	ff 75 e0             	push   -0x20(%ebp)
 8002bdb:	ff 75 0c             	push   0xc(%ebp)
 8002bde:	e8 2f dc ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 8002be3:	83 c4 20             	add    $0x20,%esp
 8002be6:	83 ec 08             	sub    $0x8,%esp
 8002be9:	68 40 40 40 00       	push   $0x404040
 8002bee:	ff 75 0c             	push   0xc(%ebp)
 8002bf1:	e8 a0 db ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002bf6:	83 c4 10             	add    $0x10,%esp
 8002bf9:	83 ec 0c             	sub    $0xc,%esp
 8002bfc:	ff 75 d4             	push   -0x2c(%ebp)
 8002bff:	ff 75 d8             	push   -0x28(%ebp)
 8002c02:	ff 75 dc             	push   -0x24(%ebp)
 8002c05:	ff 75 e0             	push   -0x20(%ebp)
 8002c08:	ff 75 0c             	push   0xc(%ebp)
 8002c0b:	e8 08 dd ff ff       	call   8000918 <_ZN8QPainter8drawRectEiiii>
 8002c10:	83 c4 20             	add    $0x20,%esp
 8002c13:	83 ec 08             	sub    $0x8,%esp
 8002c16:	68 80 80 80 00       	push   $0x808080
 8002c1b:	ff 75 0c             	push   0xc(%ebp)
 8002c1e:	e8 73 db ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002c23:	83 c4 10             	add    $0x10,%esp
 8002c26:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 8002c29:	8d 48 fe             	lea    -0x2(%eax),%ecx
 8002c2c:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8002c2f:	8d 50 01             	lea    0x1(%eax),%edx
 8002c32:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8002c35:	83 c0 01             	add    $0x1,%eax
 8002c38:	83 ec 0c             	sub    $0xc,%esp
 8002c3b:	51                   	push   %ecx
 8002c3c:	6a 1f                	push   $0x1f
 8002c3e:	52                   	push   %edx
 8002c3f:	50                   	push   %eax
 8002c40:	ff 75 0c             	push   0xc(%ebp)
 8002c43:	e8 ca db ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 8002c48:	83 c4 20             	add    $0x20,%esp
 8002c4b:	83 ec 08             	sub    $0x8,%esp
 8002c4e:	68 40 40 40 00       	push   $0x404040
 8002c53:	ff 75 0c             	push   0xc(%ebp)
 8002c56:	e8 3b db ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002c5b:	83 c4 10             	add    $0x10,%esp
 8002c5e:	8b 55 dc             	mov    -0x24(%ebp),%edx
 8002c61:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 8002c64:	01 d0                	add    %edx,%eax
 8002c66:	8d 58 fe             	lea    -0x2(%eax),%ebx
 8002c69:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8002c6c:	8d 48 20             	lea    0x20(%eax),%ecx
 8002c6f:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8002c72:	8d 50 01             	lea    0x1(%eax),%edx
 8002c75:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8002c78:	83 c0 20             	add    $0x20,%eax
 8002c7b:	83 ec 0c             	sub    $0xc,%esp
 8002c7e:	53                   	push   %ebx
 8002c7f:	51                   	push   %ecx
 8002c80:	52                   	push   %edx
 8002c81:	50                   	push   %eax
 8002c82:	ff 75 0c             	push   0xc(%ebp)
 8002c85:	e8 3c dd ff ff       	call   80009c6 <_ZN8QPainter8drawLineEiiii>
 8002c8a:	83 c4 20             	add    $0x20,%esp
 8002c8d:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8002c90:	83 c0 24             	add    $0x24,%eax
 8002c93:	89 45 c8             	mov    %eax,-0x38(%ebp)
 8002c96:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8002c99:	83 c0 02             	add    $0x2,%eax
 8002c9c:	89 45 c4             	mov    %eax,-0x3c(%ebp)
 8002c9f:	8b 45 08             	mov    0x8(%ebp),%eax
 8002ca2:	8b 40 34             	mov    0x34(%eax),%eax
 8002ca5:	83 ec 0c             	sub    $0xc,%esp
 8002ca8:	50                   	push   %eax
 8002ca9:	e8 04 04 00 00       	call   80030b2 <_ZNK13QTextDocument9lineCountEv>
 8002cae:	83 c4 10             	add    $0x10,%esp
 8002cb1:	89 45 c0             	mov    %eax,-0x40(%ebp)
 8002cb4:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8002cbb:	e9 78 02 00 00       	jmp    8002f38 <_ZN9QTextEdit10paintEventEP8QPainter+0x3e2>
 8002cc0:	8b 45 08             	mov    0x8(%ebp),%eax
 8002cc3:	8b 50 40             	mov    0x40(%eax),%edx
 8002cc6:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8002cc9:	01 d0                	add    %edx,%eax
 8002ccb:	89 45 bc             	mov    %eax,-0x44(%ebp)
 8002cce:	8b 45 bc             	mov    -0x44(%ebp),%eax
 8002cd1:	3b 45 c0             	cmp    -0x40(%ebp),%eax
 8002cd4:	0f 8d 6f 02 00 00    	jge    8002f49 <_ZN9QTextEdit10paintEventEP8QPainter+0x3f3>
 8002cda:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8002cdd:	0f af 45 d0          	imul   -0x30(%ebp),%eax
 8002ce1:	89 c2                	mov    %eax,%edx
 8002ce3:	8b 45 c4             	mov    -0x3c(%ebp),%eax
 8002ce6:	01 d0                	add    %edx,%eax
 8002ce8:	89 45 b8             	mov    %eax,-0x48(%ebp)
 8002ceb:	83 ec 08             	sub    $0x8,%esp
 8002cee:	68 40 40 40 00       	push   $0x404040
 8002cf3:	ff 75 0c             	push   0xc(%ebp)
 8002cf6:	e8 9b da ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002cfb:	83 c4 10             	add    $0x10,%esp
 8002cfe:	8b 45 bc             	mov    -0x44(%ebp),%eax
 8002d01:	83 c0 01             	add    $0x1,%eax
 8002d04:	89 45 b4             	mov    %eax,-0x4c(%ebp)
 8002d07:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8002d0e:	81 7d b4 e7 03 00 00 	cmpl   $0x3e7,-0x4c(%ebp)
 8002d15:	7e 2a                	jle    8002d41 <_ZN9QTextEdit10paintEventEP8QPainter+0x1eb>
 8002d17:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 8002d1a:	ba d3 4d 62 10       	mov    $0x10624dd3,%edx
 8002d1f:	89 c8                	mov    %ecx,%eax
 8002d21:	f7 ea                	imul   %edx
 8002d23:	c1 fa 06             	sar    $0x6,%edx
 8002d26:	89 c8                	mov    %ecx,%eax
 8002d28:	c1 f8 1f             	sar    $0x1f,%eax
 8002d2b:	29 c2                	sub    %eax,%edx
 8002d2d:	89 d0                	mov    %edx,%eax
 8002d2f:	83 c0 30             	add    $0x30,%eax
 8002d32:	89 c1                	mov    %eax,%ecx
 8002d34:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002d37:	8d 50 01             	lea    0x1(%eax),%edx
 8002d3a:	89 55 f0             	mov    %edx,-0x10(%ebp)
 8002d3d:	88 4c 05 94          	mov    %cl,-0x6c(%ebp,%eax,1)
 8002d41:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 8002d44:	ba d3 4d 62 10       	mov    $0x10624dd3,%edx
 8002d49:	89 c8                	mov    %ecx,%eax
 8002d4b:	f7 ea                	imul   %edx
 8002d4d:	c1 fa 06             	sar    $0x6,%edx
 8002d50:	89 c8                	mov    %ecx,%eax
 8002d52:	c1 f8 1f             	sar    $0x1f,%eax
 8002d55:	29 c2                	sub    %eax,%edx
 8002d57:	69 c2 e8 03 00 00    	imul   $0x3e8,%edx,%eax
 8002d5d:	29 c1                	sub    %eax,%ecx
 8002d5f:	89 ca                	mov    %ecx,%edx
 8002d61:	89 55 b4             	mov    %edx,-0x4c(%ebp)
 8002d64:	83 7d b4 63          	cmpl   $0x63,-0x4c(%ebp)
 8002d68:	7f 06                	jg     8002d70 <_ZN9QTextEdit10paintEventEP8QPainter+0x21a>
 8002d6a:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 8002d6e:	7e 2a                	jle    8002d9a <_ZN9QTextEdit10paintEventEP8QPainter+0x244>
 8002d70:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 8002d73:	ba 1f 85 eb 51       	mov    $0x51eb851f,%edx
 8002d78:	89 c8                	mov    %ecx,%eax
 8002d7a:	f7 ea                	imul   %edx
 8002d7c:	c1 fa 05             	sar    $0x5,%edx
 8002d7f:	89 c8                	mov    %ecx,%eax
 8002d81:	c1 f8 1f             	sar    $0x1f,%eax
 8002d84:	29 c2                	sub    %eax,%edx
 8002d86:	89 d0                	mov    %edx,%eax
 8002d88:	83 c0 30             	add    $0x30,%eax
 8002d8b:	89 c1                	mov    %eax,%ecx
 8002d8d:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002d90:	8d 50 01             	lea    0x1(%eax),%edx
 8002d93:	89 55 f0             	mov    %edx,-0x10(%ebp)
 8002d96:	88 4c 05 94          	mov    %cl,-0x6c(%ebp,%eax,1)
 8002d9a:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 8002d9d:	ba 1f 85 eb 51       	mov    $0x51eb851f,%edx
 8002da2:	89 c8                	mov    %ecx,%eax
 8002da4:	f7 ea                	imul   %edx
 8002da6:	c1 fa 05             	sar    $0x5,%edx
 8002da9:	89 c8                	mov    %ecx,%eax
 8002dab:	c1 f8 1f             	sar    $0x1f,%eax
 8002dae:	29 c2                	sub    %eax,%edx
 8002db0:	6b c2 64             	imul   $0x64,%edx,%eax
 8002db3:	29 c1                	sub    %eax,%ecx
 8002db5:	89 ca                	mov    %ecx,%edx
 8002db7:	89 55 b4             	mov    %edx,-0x4c(%ebp)
 8002dba:	83 7d b4 09          	cmpl   $0x9,-0x4c(%ebp)
 8002dbe:	7f 06                	jg     8002dc6 <_ZN9QTextEdit10paintEventEP8QPainter+0x270>
 8002dc0:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 8002dc4:	7e 2a                	jle    8002df0 <_ZN9QTextEdit10paintEventEP8QPainter+0x29a>
 8002dc6:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 8002dc9:	ba 67 66 66 66       	mov    $0x66666667,%edx
 8002dce:	89 c8                	mov    %ecx,%eax
 8002dd0:	f7 ea                	imul   %edx
 8002dd2:	c1 fa 02             	sar    $0x2,%edx
 8002dd5:	89 c8                	mov    %ecx,%eax
 8002dd7:	c1 f8 1f             	sar    $0x1f,%eax
 8002dda:	29 c2                	sub    %eax,%edx
 8002ddc:	89 d0                	mov    %edx,%eax
 8002dde:	83 c0 30             	add    $0x30,%eax
 8002de1:	89 c1                	mov    %eax,%ecx
 8002de3:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002de6:	8d 50 01             	lea    0x1(%eax),%edx
 8002de9:	89 55 f0             	mov    %edx,-0x10(%ebp)
 8002dec:	88 4c 05 94          	mov    %cl,-0x6c(%ebp,%eax,1)
 8002df0:	8b 4d b4             	mov    -0x4c(%ebp),%ecx
 8002df3:	ba 67 66 66 66       	mov    $0x66666667,%edx
 8002df8:	89 c8                	mov    %ecx,%eax
 8002dfa:	f7 ea                	imul   %edx
 8002dfc:	c1 fa 02             	sar    $0x2,%edx
 8002dff:	89 c8                	mov    %ecx,%eax
 8002e01:	c1 f8 1f             	sar    $0x1f,%eax
 8002e04:	29 c2                	sub    %eax,%edx
 8002e06:	89 d0                	mov    %edx,%eax
 8002e08:	c1 e0 02             	shl    $0x2,%eax
 8002e0b:	01 d0                	add    %edx,%eax
 8002e0d:	01 c0                	add    %eax,%eax
 8002e0f:	29 c1                	sub    %eax,%ecx
 8002e11:	89 ca                	mov    %ecx,%edx
 8002e13:	89 55 b4             	mov    %edx,-0x4c(%ebp)
 8002e16:	8b 45 b4             	mov    -0x4c(%ebp),%eax
 8002e19:	83 c0 30             	add    $0x30,%eax
 8002e1c:	89 c1                	mov    %eax,%ecx
 8002e1e:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002e21:	8d 50 01             	lea    0x1(%eax),%edx
 8002e24:	89 55 f0             	mov    %edx,-0x10(%ebp)
 8002e27:	88 4c 05 94          	mov    %cl,-0x6c(%ebp,%eax,1)
 8002e2b:	8d 55 94             	lea    -0x6c(%ebp),%edx
 8002e2e:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002e31:	01 d0                	add    %edx,%eax
 8002e33:	c6 00 00             	movb   $0x0,(%eax)
 8002e36:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8002e39:	0f af 45 cc          	imul   -0x34(%ebp),%eax
 8002e3d:	89 45 b0             	mov    %eax,-0x50(%ebp)
 8002e40:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8002e43:	83 c0 1c             	add    $0x1c,%eax
 8002e46:	2b 45 b0             	sub    -0x50(%ebp),%eax
 8002e49:	8d 55 94             	lea    -0x6c(%ebp),%edx
 8002e4c:	52                   	push   %edx
 8002e4d:	ff 75 b8             	push   -0x48(%ebp)
 8002e50:	50                   	push   %eax
 8002e51:	ff 75 0c             	push   0xc(%ebp)
 8002e54:	e8 d1 dc ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 8002e59:	83 c4 10             	add    $0x10,%esp
 8002e5c:	8b 45 08             	mov    0x8(%ebp),%eax
 8002e5f:	8b 40 34             	mov    0x34(%eax),%eax
 8002e62:	83 ec 08             	sub    $0x8,%esp
 8002e65:	ff 75 bc             	push   -0x44(%ebp)
 8002e68:	50                   	push   %eax
 8002e69:	e8 14 e9 ff ff       	call   8001782 <_ZNK13QTextDocument8lineTextEi>
 8002e6e:	83 c4 10             	add    $0x10,%esp
 8002e71:	89 45 ac             	mov    %eax,-0x54(%ebp)
 8002e74:	8b 45 08             	mov    0x8(%ebp),%eax
 8002e77:	8b 40 34             	mov    0x34(%eax),%eax
 8002e7a:	83 ec 08             	sub    $0x8,%esp
 8002e7d:	ff 75 bc             	push   -0x44(%ebp)
 8002e80:	50                   	push   %eax
 8002e81:	e8 c8 e8 ff ff       	call   800174e <_ZNK13QTextDocument10lineLengthEi>
 8002e86:	83 c4 10             	add    $0x10,%esp
 8002e89:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8002e8c:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8002e8f:	83 e8 28             	sub    $0x28,%eax
 8002e92:	99                   	cltd
 8002e93:	f7 7d cc             	idivl  -0x34(%ebp)
 8002e96:	89 45 a8             	mov    %eax,-0x58(%ebp)
 8002e99:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8002e9c:	3b 45 a8             	cmp    -0x58(%ebp),%eax
 8002e9f:	7e 06                	jle    8002ea7 <_ZN9QTextEdit10paintEventEP8QPainter+0x351>
 8002ea1:	8b 45 a8             	mov    -0x58(%ebp),%eax
 8002ea4:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8002ea7:	83 ec 08             	sub    $0x8,%esp
 8002eaa:	6a 00                	push   $0x0
 8002eac:	ff 75 0c             	push   0xc(%ebp)
 8002eaf:	e8 e2 d8 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002eb4:	83 c4 10             	add    $0x10,%esp
 8002eb7:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
 8002ebe:	eb 6c                	jmp    8002f2c <_ZN9QTextEdit10paintEventEP8QPainter+0x3d6>
 8002ec0:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8002ec3:	8b 45 ac             	mov    -0x54(%ebp),%eax
 8002ec6:	01 d0                	add    %edx,%eax
 8002ec8:	0f b6 00             	movzbl (%eax),%eax
 8002ecb:	3c 09                	cmp    $0x9,%al
 8002ecd:	75 24                	jne    8002ef3 <_ZN9QTextEdit10paintEventEP8QPainter+0x39d>
 8002ecf:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8002ed2:	0f af 45 cc          	imul   -0x34(%ebp),%eax
 8002ed6:	89 c2                	mov    %eax,%edx
 8002ed8:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8002edb:	01 d0                	add    %edx,%eax
 8002edd:	68 2c 53 00 08       	push   $0x800532c
 8002ee2:	ff 75 b8             	push   -0x48(%ebp)
 8002ee5:	50                   	push   %eax
 8002ee6:	ff 75 0c             	push   0xc(%ebp)
 8002ee9:	e8 3c dc ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 8002eee:	83 c4 10             	add    $0x10,%esp
 8002ef1:	eb 35                	jmp    8002f28 <_ZN9QTextEdit10paintEventEP8QPainter+0x3d2>
 8002ef3:	66 c7 45 92 00 00    	movw   $0x0,-0x6e(%ebp)
 8002ef9:	8b 55 e8             	mov    -0x18(%ebp),%edx
 8002efc:	8b 45 ac             	mov    -0x54(%ebp),%eax
 8002eff:	01 d0                	add    %edx,%eax
 8002f01:	0f b6 00             	movzbl (%eax),%eax
 8002f04:	88 45 92             	mov    %al,-0x6e(%ebp)
 8002f07:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8002f0a:	0f af 45 cc          	imul   -0x34(%ebp),%eax
 8002f0e:	89 c2                	mov    %eax,%edx
 8002f10:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8002f13:	01 c2                	add    %eax,%edx
 8002f15:	8d 45 92             	lea    -0x6e(%ebp),%eax
 8002f18:	50                   	push   %eax
 8002f19:	ff 75 b8             	push   -0x48(%ebp)
 8002f1c:	52                   	push   %edx
 8002f1d:	ff 75 0c             	push   0xc(%ebp)
 8002f20:	e8 05 dc ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 8002f25:	83 c4 10             	add    $0x10,%esp
 8002f28:	83 45 e8 01          	addl   $0x1,-0x18(%ebp)
 8002f2c:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8002f2f:	3b 45 ec             	cmp    -0x14(%ebp),%eax
 8002f32:	7c 8c                	jl     8002ec0 <_ZN9QTextEdit10paintEventEP8QPainter+0x36a>
 8002f34:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8002f38:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f3b:	8b 40 44             	mov    0x44(%eax),%eax
 8002f3e:	39 45 f4             	cmp    %eax,-0xc(%ebp)
 8002f41:	0f 8c 79 fd ff ff    	jl     8002cc0 <_ZN9QTextEdit10paintEventEP8QPainter+0x16a>
 8002f47:	eb 01                	jmp    8002f4a <_ZN9QTextEdit10paintEventEP8QPainter+0x3f4>
 8002f49:	90                   	nop
 8002f4a:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f4d:	8b 50 38             	mov    0x38(%eax),%edx
 8002f50:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f53:	8b 40 40             	mov    0x40(%eax),%eax
 8002f56:	39 c2                	cmp    %eax,%edx
 8002f58:	0f 8c a3 00 00 00    	jl     8003001 <_ZN9QTextEdit10paintEventEP8QPainter+0x4ab>
 8002f5e:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f61:	8b 50 38             	mov    0x38(%eax),%edx
 8002f64:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f67:	8b 48 40             	mov    0x40(%eax),%ecx
 8002f6a:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f6d:	8b 40 44             	mov    0x44(%eax),%eax
 8002f70:	01 c8                	add    %ecx,%eax
 8002f72:	39 c2                	cmp    %eax,%edx
 8002f74:	0f 8d 87 00 00 00    	jge    8003001 <_ZN9QTextEdit10paintEventEP8QPainter+0x4ab>
 8002f7a:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f7d:	8b 50 38             	mov    0x38(%eax),%edx
 8002f80:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f83:	8b 40 40             	mov    0x40(%eax),%eax
 8002f86:	29 c2                	sub    %eax,%edx
 8002f88:	0f af 55 d0          	imul   -0x30(%ebp),%edx
 8002f8c:	8b 45 c4             	mov    -0x3c(%ebp),%eax
 8002f8f:	01 d0                	add    %edx,%eax
 8002f91:	89 45 a4             	mov    %eax,-0x5c(%ebp)
 8002f94:	8b 45 08             	mov    0x8(%ebp),%eax
 8002f97:	8b 40 3c             	mov    0x3c(%eax),%eax
 8002f9a:	0f af 45 cc          	imul   -0x34(%ebp),%eax
 8002f9e:	89 c2                	mov    %eax,%edx
 8002fa0:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8002fa3:	01 d0                	add    %edx,%eax
 8002fa5:	89 45 a0             	mov    %eax,-0x60(%ebp)
 8002fa8:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8002fab:	8d 50 d8             	lea    -0x28(%eax),%edx
 8002fae:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8002fb1:	01 d0                	add    %edx,%eax
 8002fb3:	39 45 a0             	cmp    %eax,-0x60(%ebp)
 8002fb6:	7d 49                	jge    8003001 <_ZN9QTextEdit10paintEventEP8QPainter+0x4ab>
 8002fb8:	8b 45 08             	mov    0x8(%ebp),%eax
 8002fbb:	8b 40 44             	mov    0x44(%eax),%eax
 8002fbe:	0f af 45 d0          	imul   -0x30(%ebp),%eax
 8002fc2:	89 c2                	mov    %eax,%edx
 8002fc4:	8b 45 c4             	mov    -0x3c(%ebp),%eax
 8002fc7:	01 d0                	add    %edx,%eax
 8002fc9:	39 45 a4             	cmp    %eax,-0x5c(%ebp)
 8002fcc:	7d 33                	jge    8003001 <_ZN9QTextEdit10paintEventEP8QPainter+0x4ab>
 8002fce:	83 ec 08             	sub    $0x8,%esp
 8002fd1:	6a 00                	push   $0x0
 8002fd3:	ff 75 0c             	push   0xc(%ebp)
 8002fd6:	e8 bb d7 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8002fdb:	83 c4 10             	add    $0x10,%esp
 8002fde:	8b 55 a4             	mov    -0x5c(%ebp),%edx
 8002fe1:	8b 45 d0             	mov    -0x30(%ebp),%eax
 8002fe4:	01 d0                	add    %edx,%eax
 8002fe6:	83 e8 01             	sub    $0x1,%eax
 8002fe9:	83 ec 0c             	sub    $0xc,%esp
 8002fec:	50                   	push   %eax
 8002fed:	ff 75 a0             	push   -0x60(%ebp)
 8002ff0:	ff 75 a4             	push   -0x5c(%ebp)
 8002ff3:	ff 75 a0             	push   -0x60(%ebp)
 8002ff6:	ff 75 0c             	push   0xc(%ebp)
 8002ff9:	e8 c8 d9 ff ff       	call   80009c6 <_ZN8QPainter8drawLineEiiii>
 8002ffe:	83 c4 20             	add    $0x20,%esp
 8003001:	8b 45 08             	mov    0x8(%ebp),%eax
 8003004:	8b 40 40             	mov    0x40(%eax),%eax
 8003007:	85 c0                	test   %eax,%eax
 8003009:	7f 0f                	jg     800301a <_ZN9QTextEdit10paintEventEP8QPainter+0x4c4>
 800300b:	8b 45 08             	mov    0x8(%ebp),%eax
 800300e:	8b 40 44             	mov    0x44(%eax),%eax
 8003011:	39 45 c0             	cmp    %eax,-0x40(%ebp)
 8003014:	0f 8e 92 00 00 00    	jle    80030ac <_ZN9QTextEdit10paintEventEP8QPainter+0x556>
 800301a:	8b 45 08             	mov    0x8(%ebp),%eax
 800301d:	8b 40 44             	mov    0x44(%eax),%eax
 8003020:	39 45 c0             	cmp    %eax,-0x40(%ebp)
 8003023:	0f 8e 83 00 00 00    	jle    80030ac <_ZN9QTextEdit10paintEventEP8QPainter+0x556>
 8003029:	8b 45 08             	mov    0x8(%ebp),%eax
 800302c:	8b 40 44             	mov    0x44(%eax),%eax
 800302f:	8b 55 d4             	mov    -0x2c(%ebp),%edx
 8003032:	83 ea 04             	sub    $0x4,%edx
 8003035:	0f af c2             	imul   %edx,%eax
 8003038:	99                   	cltd
 8003039:	f7 7d c0             	idivl  -0x40(%ebp)
 800303c:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 800303f:	83 7d e4 07          	cmpl   $0x7,-0x1c(%ebp)
 8003043:	7f 07                	jg     800304c <_ZN9QTextEdit10paintEventEP8QPainter+0x4f6>
 8003045:	c7 45 e4 08 00 00 00 	movl   $0x8,-0x1c(%ebp)
 800304c:	8b 45 dc             	mov    -0x24(%ebp),%eax
 800304f:	8d 58 02             	lea    0x2(%eax),%ebx
 8003052:	8b 45 08             	mov    0x8(%ebp),%eax
 8003055:	8b 50 40             	mov    0x40(%eax),%edx
 8003058:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 800305b:	83 e8 04             	sub    $0x4,%eax
 800305e:	2b 45 e4             	sub    -0x1c(%ebp),%eax
 8003061:	0f af c2             	imul   %edx,%eax
 8003064:	8b 55 08             	mov    0x8(%ebp),%edx
 8003067:	8b 52 44             	mov    0x44(%edx),%edx
 800306a:	8b 4d c0             	mov    -0x40(%ebp),%ecx
 800306d:	29 d1                	sub    %edx,%ecx
 800306f:	99                   	cltd
 8003070:	f7 f9                	idiv   %ecx
 8003072:	01 d8                	add    %ebx,%eax
 8003074:	89 45 9c             	mov    %eax,-0x64(%ebp)
 8003077:	83 ec 08             	sub    $0x8,%esp
 800307a:	68 80 80 80 00       	push   $0x808080
 800307f:	ff 75 0c             	push   0xc(%ebp)
 8003082:	e8 0f d7 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8003087:	83 c4 10             	add    $0x10,%esp
 800308a:	8b 55 e0             	mov    -0x20(%ebp),%edx
 800308d:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8003090:	01 d0                	add    %edx,%eax
 8003092:	83 e8 08             	sub    $0x8,%eax
 8003095:	83 ec 0c             	sub    $0xc,%esp
 8003098:	ff 75 e4             	push   -0x1c(%ebp)
 800309b:	6a 06                	push   $0x6
 800309d:	ff 75 9c             	push   -0x64(%ebp)
 80030a0:	50                   	push   %eax
 80030a1:	ff 75 0c             	push   0xc(%ebp)
 80030a4:	e8 69 d7 ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 80030a9:	83 c4 20             	add    $0x20,%esp
 80030ac:	90                   	nop
 80030ad:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80030b0:	c9                   	leave
 80030b1:	c3                   	ret

080030b2 <_ZNK13QTextDocument9lineCountEv>:
 80030b2:	55                   	push   %ebp
 80030b3:	89 e5                	mov    %esp,%ebp
 80030b5:	8b 45 08             	mov    0x8(%ebp),%eax
 80030b8:	8b 40 04             	mov    0x4(%eax),%eax
 80030bb:	5d                   	pop    %ebp
 80030bc:	c3                   	ret
 80030bd:	90                   	nop

080030be <_ZNK9QTextEdit9classNameEv>:
 80030be:	55                   	push   %ebp
 80030bf:	89 e5                	mov    %esp,%ebp
 80030c1:	b8 c0 50 00 08       	mov    $0x80050c0,%eax
 80030c6:	5d                   	pop    %ebp
 80030c7:	c3                   	ret

080030c8 <gui_get_fb_info>:
 80030c8:	55                   	push   %ebp
 80030c9:	89 e5                	mov    %esp,%ebp
 80030cb:	53                   	push   %ebx
 80030cc:	83 ec 10             	sub    $0x10,%esp
 80030cf:	b8 46 00 00 00       	mov    $0x46,%eax
 80030d4:	8b 55 08             	mov    0x8(%ebp),%edx
 80030d7:	89 d3                	mov    %edx,%ebx
 80030d9:	cd 80                	int    $0x80
 80030db:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80030de:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80030e1:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80030e4:	c9                   	leave
 80030e5:	c3                   	ret

080030e6 <_ZL10read_inputP13input_event_ti>:
 80030e6:	55                   	push   %ebp
 80030e7:	89 e5                	mov    %esp,%ebp
 80030e9:	53                   	push   %ebx
 80030ea:	83 ec 10             	sub    $0x10,%esp
 80030ed:	b8 48 00 00 00       	mov    $0x48,%eax
 80030f2:	8b 55 08             	mov    0x8(%ebp),%edx
 80030f5:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 80030f8:	89 d3                	mov    %edx,%ebx
 80030fa:	cd 80                	int    $0x80
 80030fc:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80030ff:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003102:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003105:	c9                   	leave
 8003106:	c3                   	ret

08003107 <_ZL9yield_cpuv>:
 8003107:	55                   	push   %ebp
 8003108:	89 e5                	mov    %esp,%ebp
 800310a:	b8 03 00 00 00       	mov    $0x3,%eax
 800310f:	cd 80                	int    $0x80
 8003111:	90                   	nop
 8003112:	5d                   	pop    %ebp
 8003113:	c3                   	ret

08003114 <main>:
 8003114:	8d 4c 24 04          	lea    0x4(%esp),%ecx
 8003118:	83 e4 f0             	and    $0xfffffff0,%esp
 800311b:	ff 71 fc             	push   -0x4(%ecx)
 800311e:	55                   	push   %ebp
 800311f:	89 e5                	mov    %esp,%ebp
 8003121:	53                   	push   %ebx
 8003122:	51                   	push   %ecx
 8003123:	81 ec 00 01 00 00    	sub    $0x100,%esp
 8003129:	83 ec 0c             	sub    $0xc,%esp
 800312c:	68 48 53 00 08       	push   $0x8005348
 8003131:	e8 4b 0b 00 00       	call   8003c81 <printf>
 8003136:	83 c4 10             	add    $0x10,%esp
 8003139:	83 ec 0c             	sub    $0xc,%esp
 800313c:	68 4c 53 00 08       	push   $0x800534c
 8003141:	e8 3b 0b 00 00       	call   8003c81 <printf>
 8003146:	83 c4 10             	add    $0x10,%esp
 8003149:	83 ec 0c             	sub    $0xc,%esp
 800314c:	68 78 53 00 08       	push   $0x8005378
 8003151:	e8 2b 0b 00 00       	call   8003c81 <printf>
 8003156:	83 c4 10             	add    $0x10,%esp
 8003159:	83 ec 0c             	sub    $0xc,%esp
 800315c:	68 4c 53 00 08       	push   $0x800534c
 8003161:	e8 1b 0b 00 00       	call   8003c81 <printf>
 8003166:	83 c4 10             	add    $0x10,%esp
 8003169:	83 ec 0c             	sub    $0xc,%esp
 800316c:	68 98 53 00 08       	push   $0x8005398
 8003171:	e8 0b 0b 00 00       	call   8003c81 <printf>
 8003176:	83 c4 10             	add    $0x10,%esp
 8003179:	83 ec 0c             	sub    $0xc,%esp
 800317c:	8d 45 98             	lea    -0x68(%ebp),%eax
 800317f:	50                   	push   %eax
 8003180:	e8 43 ff ff ff       	call   80030c8 <gui_get_fb_info>
 8003185:	83 c4 10             	add    $0x10,%esp
 8003188:	85 c0                	test   %eax,%eax
 800318a:	0f 95 c0             	setne  %al
 800318d:	84 c0                	test   %al,%al
 800318f:	74 1a                	je     80031ab <main+0x97>
 8003191:	83 ec 0c             	sub    $0xc,%esp
 8003194:	68 b8 53 00 08       	push   $0x80053b8
 8003199:	e8 e3 0a 00 00       	call   8003c81 <printf>
 800319e:	83 c4 10             	add    $0x10,%esp
 80031a1:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 80031a6:	e9 00 07 00 00       	jmp    80038ab <main+0x797>
 80031ab:	8b 55 a0             	mov    -0x60(%ebp),%edx
 80031ae:	8b 45 9c             	mov    -0x64(%ebp),%eax
 80031b1:	83 ec 04             	sub    $0x4,%esp
 80031b4:	52                   	push   %edx
 80031b5:	50                   	push   %eax
 80031b6:	68 d9 53 00 08       	push   $0x80053d9
 80031bb:	e8 c1 0a 00 00       	call   8003c81 <printf>
 80031c0:	83 c4 10             	add    $0x10,%esp
 80031c3:	c7 45 cc 00 00 00 f0 	movl   $0xf0000000,-0x34(%ebp)
 80031ca:	83 ec 0c             	sub    $0xc,%esp
 80031cd:	68 ef 53 00 08       	push   $0x80053ef
 80031d2:	e8 aa 0a 00 00       	call   8003c81 <printf>
 80031d7:	83 c4 10             	add    $0x10,%esp
 80031da:	8b 45 a4             	mov    -0x5c(%ebp),%eax
 80031dd:	89 c1                	mov    %eax,%ecx
 80031df:	8b 45 a0             	mov    -0x60(%ebp),%eax
 80031e2:	89 c2                	mov    %eax,%edx
 80031e4:	8b 45 9c             	mov    -0x64(%ebp),%eax
 80031e7:	83 ec 0c             	sub    $0xc,%esp
 80031ea:	51                   	push   %ecx
 80031eb:	52                   	push   %edx
 80031ec:	50                   	push   %eax
 80031ed:	ff 75 cc             	push   -0x34(%ebp)
 80031f0:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 80031f6:	50                   	push   %eax
 80031f7:	e8 22 d5 ff ff       	call   800071e <_ZN8QPainterC1EPjiii>
 80031fc:	83 c4 20             	add    $0x20,%esp
 80031ff:	8b 45 a4             	mov    -0x5c(%ebp),%eax
 8003202:	c1 e8 02             	shr    $0x2,%eax
 8003205:	83 ec 08             	sub    $0x8,%esp
 8003208:	50                   	push   %eax
 8003209:	68 08 54 00 08       	push   $0x8005408
 800320e:	e8 6e 0a 00 00       	call   8003c81 <printf>
 8003213:	83 c4 10             	add    $0x10,%esp
 8003216:	83 ec 08             	sub    $0x8,%esp
 8003219:	68 40 40 40 00       	push   $0x404040
 800321e:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 8003224:	50                   	push   %eax
 8003225:	e8 9a d9 ff ff       	call   8000bc4 <_ZN8QPainter5clearEj>
 800322a:	83 c4 10             	add    $0x10,%esp
 800322d:	83 ec 0c             	sub    $0xc,%esp
 8003230:	68 33 54 00 08       	push   $0x8005433
 8003235:	e8 47 0a 00 00       	call   8003c81 <printf>
 800323a:	83 c4 10             	add    $0x10,%esp
 800323d:	83 ec 0c             	sub    $0xc,%esp
 8003240:	6a 1c                	push   $0x1c
 8003242:	e8 43 ce ff ff       	call   800008a <_Znwj>
 8003247:	83 c4 10             	add    $0x10,%esp
 800324a:	89 c3                	mov    %eax,%ebx
 800324c:	83 ec 04             	sub    $0x4,%esp
 800324f:	68 4f 54 00 08       	push   $0x800544f
 8003254:	6a 00                	push   $0x0
 8003256:	53                   	push   %ebx
 8003257:	e8 dc cf ff ff       	call   8000238 <_ZN7QObjectC1EPS_PKc>
 800325c:	83 c4 10             	add    $0x10,%esp
 800325f:	89 5d c8             	mov    %ebx,-0x38(%ebp)
 8003262:	83 ec 0c             	sub    $0xc,%esp
 8003265:	68 54 54 00 08       	push   $0x8005454
 800326a:	e8 12 0a 00 00       	call   8003c81 <printf>
 800326f:	83 c4 10             	add    $0x10,%esp
 8003272:	83 ec 0c             	sub    $0xc,%esp
 8003275:	6a 34                	push   $0x34
 8003277:	e8 0e ce ff ff       	call   800008a <_Znwj>
 800327c:	83 c4 10             	add    $0x10,%esp
 800327f:	89 c3                	mov    %eax,%ebx
 8003281:	83 ec 04             	sub    $0x4,%esp
 8003284:	68 6d 54 00 08       	push   $0x800546d
 8003289:	6a 00                	push   $0x0
 800328b:	53                   	push   %ebx
 800328c:	e8 8f da ff ff       	call   8000d20 <_ZN7QWidgetC1EPS_PKc>
 8003291:	83 c4 10             	add    $0x10,%esp
 8003294:	89 5d c4             	mov    %ebx,-0x3c(%ebp)
 8003297:	83 ec 0c             	sub    $0xc,%esp
 800329a:	68 73 54 00 08       	push   $0x8005473
 800329f:	e8 dd 09 00 00       	call   8003c81 <printf>
 80032a4:	83 c4 10             	add    $0x10,%esp
 80032a7:	83 ec 0c             	sub    $0xc,%esp
 80032aa:	6a 0c                	push   $0xc
 80032ac:	e8 d9 cd ff ff       	call   800008a <_Znwj>
 80032b1:	83 c4 10             	add    $0x10,%esp
 80032b4:	89 c3                	mov    %eax,%ebx
 80032b6:	83 ec 0c             	sub    $0xc,%esp
 80032b9:	53                   	push   %ebx
 80032ba:	e8 81 e3 ff ff       	call   8001640 <_ZN13QTextDocumentC1Ev>
 80032bf:	83 c4 10             	add    $0x10,%esp
 80032c2:	89 5d c0             	mov    %ebx,-0x40(%ebp)
 80032c5:	83 ec 0c             	sub    $0xc,%esp
 80032c8:	68 8c 54 00 08       	push   $0x800548c
 80032cd:	e8 af 09 00 00       	call   8003c81 <printf>
 80032d2:	83 c4 10             	add    $0x10,%esp
 80032d5:	83 ec 08             	sub    $0x8,%esp
 80032d8:	68 b1 54 00 08       	push   $0x80054b1
 80032dd:	ff 75 c0             	push   -0x40(%ebp)
 80032e0:	e8 eb ee ff ff       	call   80021d0 <_ZN13QTextDocument12setPlainTextEPKc>
 80032e5:	83 c4 10             	add    $0x10,%esp
 80032e8:	83 ec 0c             	sub    $0xc,%esp
 80032eb:	ff 75 c0             	push   -0x40(%ebp)
 80032ee:	e8 bf fd ff ff       	call   80030b2 <_ZNK13QTextDocument9lineCountEv>
 80032f3:	83 c4 10             	add    $0x10,%esp
 80032f6:	83 ec 08             	sub    $0x8,%esp
 80032f9:	50                   	push   %eax
 80032fa:	68 c0 54 00 08       	push   $0x80054c0
 80032ff:	e8 7d 09 00 00       	call   8003c81 <printf>
 8003304:	83 c4 10             	add    $0x10,%esp
 8003307:	c7 45 bc f0 54 00 08 	movl   $0x80054f0,-0x44(%ebp)
 800330e:	83 ec 08             	sub    $0x8,%esp
 8003311:	ff 75 bc             	push   -0x44(%ebp)
 8003314:	ff 75 c0             	push   -0x40(%ebp)
 8003317:	e8 b4 ee ff ff       	call   80021d0 <_ZN13QTextDocument12setPlainTextEPKc>
 800331c:	83 c4 10             	add    $0x10,%esp
 800331f:	83 ec 0c             	sub    $0xc,%esp
 8003322:	ff 75 c0             	push   -0x40(%ebp)
 8003325:	e8 88 fd ff ff       	call   80030b2 <_ZNK13QTextDocument9lineCountEv>
 800332a:	83 c4 10             	add    $0x10,%esp
 800332d:	83 ec 08             	sub    $0x8,%esp
 8003330:	50                   	push   %eax
 8003331:	68 d0 55 00 08       	push   $0x80055d0
 8003336:	e8 46 09 00 00       	call   8003c81 <printf>
 800333b:	83 c4 10             	add    $0x10,%esp
 800333e:	83 ec 0c             	sub    $0xc,%esp
 8003341:	6a 48                	push   $0x48
 8003343:	e8 42 cd ff ff       	call   800008a <_Znwj>
 8003348:	83 c4 10             	add    $0x10,%esp
 800334b:	89 c3                	mov    %eax,%ebx
 800334d:	83 ec 04             	sub    $0x4,%esp
 8003350:	68 fe 55 00 08       	push   $0x80055fe
 8003355:	6a 00                	push   $0x0
 8003357:	53                   	push   %ebx
 8003358:	e8 d7 f1 ff ff       	call   8002534 <_ZN9QTextEditC1EP7QWidgetPKc>
 800335d:	83 c4 10             	add    $0x10,%esp
 8003360:	89 5d b8             	mov    %ebx,-0x48(%ebp)
 8003363:	83 ec 0c             	sub    $0xc,%esp
 8003366:	68 05 56 00 08       	push   $0x8005605
 800336b:	e8 11 09 00 00       	call   8003c81 <printf>
 8003370:	83 c4 10             	add    $0x10,%esp
 8003373:	8b 45 a0             	mov    -0x60(%ebp),%eax
 8003376:	83 e8 18             	sub    $0x18,%eax
 8003379:	89 45 b4             	mov    %eax,-0x4c(%ebp)
 800337c:	83 ec 08             	sub    $0x8,%esp
 800337f:	68 80 00 00 00       	push   $0x80
 8003384:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 800338a:	50                   	push   %eax
 800338b:	e8 06 d4 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8003390:	83 c4 10             	add    $0x10,%esp
 8003393:	8b 45 9c             	mov    -0x64(%ebp),%eax
 8003396:	83 ec 0c             	sub    $0xc,%esp
 8003399:	6a 18                	push   $0x18
 800339b:	50                   	push   %eax
 800339c:	ff 75 b4             	push   -0x4c(%ebp)
 800339f:	6a 00                	push   $0x0
 80033a1:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 80033a7:	50                   	push   %eax
 80033a8:	e8 65 d4 ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 80033ad:	83 c4 20             	add    $0x20,%esp
 80033b0:	83 ec 08             	sub    $0x8,%esp
 80033b3:	68 ff ff ff 00       	push   $0xffffff
 80033b8:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 80033be:	50                   	push   %eax
 80033bf:	e8 d2 d3 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 80033c4:	83 c4 10             	add    $0x10,%esp
 80033c7:	8b 45 b4             	mov    -0x4c(%ebp),%eax
 80033ca:	83 c0 08             	add    $0x8,%eax
 80033cd:	68 20 56 00 08       	push   $0x8005620
 80033d2:	50                   	push   %eax
 80033d3:	6a 08                	push   $0x8
 80033d5:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 80033db:	50                   	push   %eax
 80033dc:	e8 49 d7 ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 80033e1:	83 c4 10             	add    $0x10,%esp
 80033e4:	8b 45 b8             	mov    -0x48(%ebp),%eax
 80033e7:	6a 00                	push   $0x0
 80033e9:	6a 00                	push   $0x0
 80033eb:	8d 95 6c ff ff ff    	lea    -0x94(%ebp),%edx
 80033f1:	52                   	push   %edx
 80033f2:	50                   	push   %eax
 80033f3:	e8 a4 da ff ff       	call   8000e9c <_ZN7QWidget6renderEP8QPainterii>
 80033f8:	83 c4 10             	add    $0x10,%esp
 80033fb:	83 ec 0c             	sub    $0xc,%esp
 80033fe:	68 5c 56 00 08       	push   $0x800565c
 8003403:	e8 79 08 00 00       	call   8003c81 <printf>
 8003408:	83 c4 10             	add    $0x10,%esp
 800340b:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 800340f:	c6 45 f6 00          	movb   $0x0,-0xa(%ebp)
 8003413:	83 ec 0c             	sub    $0xc,%esp
 8003416:	68 90 56 00 08       	push   $0x8005690
 800341b:	e8 61 08 00 00       	call   8003c81 <printf>
 8003420:	83 c4 10             	add    $0x10,%esp
 8003423:	e8 df fc ff ff       	call   8003107 <_ZL9yield_cpuv>
 8003428:	83 ec 08             	sub    $0x8,%esp
 800342b:	6a 01                	push   $0x1
 800342d:	8d 85 5c ff ff ff    	lea    -0xa4(%ebp),%eax
 8003433:	50                   	push   %eax
 8003434:	e8 ad fc ff ff       	call   80030e6 <_ZL10read_inputP13input_event_ti>
 8003439:	83 c4 10             	add    $0x10,%esp
 800343c:	89 45 b0             	mov    %eax,-0x50(%ebp)
 800343f:	83 7d b0 00          	cmpl   $0x0,-0x50(%ebp)
 8003443:	0f 8e 11 04 00 00    	jle    800385a <main+0x746>
 8003449:	8b 85 60 ff ff ff    	mov    -0xa0(%ebp),%eax
 800344f:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8003452:	8b 85 68 ff ff ff    	mov    -0x98(%ebp),%eax
 8003458:	89 45 ac             	mov    %eax,-0x54(%ebp)
 800345b:	83 7d ac 00          	cmpl   $0x0,-0x54(%ebp)
 800345f:	0f 84 fb 03 00 00    	je     8003860 <main+0x74c>
 8003465:	81 7d f0 e0 00 00 00 	cmpl   $0xe0,-0x10(%ebp)
 800346c:	75 09                	jne    8003477 <main+0x363>
 800346e:	c6 45 f7 01          	movb   $0x1,-0x9(%ebp)
 8003472:	e9 ea 03 00 00       	jmp    8003861 <main+0x74d>
 8003477:	80 7d f7 00          	cmpb   $0x0,-0x9(%ebp)
 800347b:	74 0b                	je     8003488 <main+0x374>
 800347d:	81 4d f0 00 e0 00 00 	orl    $0xe000,-0x10(%ebp)
 8003484:	c6 45 f7 00          	movb   $0x0,-0x9(%ebp)
 8003488:	83 7d f0 12          	cmpl   $0x12,-0x10(%ebp)
 800348c:	74 06                	je     8003494 <main+0x380>
 800348e:	83 7d f0 59          	cmpl   $0x59,-0x10(%ebp)
 8003492:	75 09                	jne    800349d <main+0x389>
 8003494:	c6 45 f6 01          	movb   $0x1,-0xa(%ebp)
 8003498:	e9 c4 03 00 00       	jmp    8003861 <main+0x74d>
 800349d:	83 7d f0 76          	cmpl   $0x76,-0x10(%ebp)
 80034a1:	75 20                	jne    80034c3 <main+0x3af>
 80034a3:	83 ec 0c             	sub    $0xc,%esp
 80034a6:	68 b4 56 00 08       	push   $0x80056b4
 80034ab:	e8 d1 07 00 00       	call   8003c81 <printf>
 80034b0:	83 c4 10             	add    $0x10,%esp
 80034b3:	8b 45 b8             	mov    -0x48(%ebp),%eax
 80034b6:	85 c0                	test   %eax,%eax
 80034b8:	0f 85 a8 03 00 00    	jne    8003866 <main+0x752>
 80034be:	e9 b3 03 00 00       	jmp    8003876 <main+0x762>
 80034c3:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80034c6:	89 45 ec             	mov    %eax,-0x14(%ebp)
 80034c9:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80034cc:	25 00 e0 00 00       	and    $0xe000,%eax
 80034d1:	85 c0                	test   %eax,%eax
 80034d3:	74 09                	je     80034de <main+0x3ca>
 80034d5:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80034d8:	0f b6 c0             	movzbl %al,%eax
 80034db:	89 45 ec             	mov    %eax,-0x14(%ebp)
 80034de:	0f b6 45 f6          	movzbl -0xa(%ebp),%eax
 80034e2:	83 ec 04             	sub    $0x4,%esp
 80034e5:	50                   	push   %eax
 80034e6:	ff 75 ec             	push   -0x14(%ebp)
 80034e9:	ff 75 b8             	push   -0x48(%ebp)
 80034ec:	e8 05 f4 ff ff       	call   80028f6 <_ZN9QTextEdit8keyPressEib>
 80034f1:	83 c4 10             	add    $0x10,%esp
 80034f4:	8b 45 b8             	mov    -0x48(%ebp),%eax
 80034f7:	6a 00                	push   $0x0
 80034f9:	6a 00                	push   $0x0
 80034fb:	8d 95 6c ff ff ff    	lea    -0x94(%ebp),%edx
 8003501:	52                   	push   %edx
 8003502:	50                   	push   %eax
 8003503:	e8 94 d9 ff ff       	call   8000e9c <_ZN7QWidget6renderEP8QPainterii>
 8003508:	83 c4 10             	add    $0x10,%esp
 800350b:	83 ec 08             	sub    $0x8,%esp
 800350e:	68 80 00 00 00       	push   $0x80
 8003513:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 8003519:	50                   	push   %eax
 800351a:	e8 77 d2 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 800351f:	83 c4 10             	add    $0x10,%esp
 8003522:	8b 45 9c             	mov    -0x64(%ebp),%eax
 8003525:	83 ec 0c             	sub    $0xc,%esp
 8003528:	6a 18                	push   $0x18
 800352a:	50                   	push   %eax
 800352b:	ff 75 b4             	push   -0x4c(%ebp)
 800352e:	6a 00                	push   $0x0
 8003530:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 8003536:	50                   	push   %eax
 8003537:	e8 d6 d2 ff ff       	call   8000812 <_ZN8QPainter8fillRectEiiii>
 800353c:	83 c4 20             	add    $0x20,%esp
 800353f:	83 ec 08             	sub    $0x8,%esp
 8003542:	68 ff ff ff 00       	push   $0xffffff
 8003547:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 800354d:	50                   	push   %eax
 800354e:	e8 43 d2 ff ff       	call   8000796 <_ZN8QPainter8setColorEj>
 8003553:	83 c4 10             	add    $0x10,%esp
 8003556:	c7 45 e8 db 56 00 08 	movl   $0x80056db,-0x18(%ebp)
 800355d:	c7 45 e4 00 00 00 00 	movl   $0x0,-0x1c(%ebp)
 8003564:	eb 1c                	jmp    8003582 <main+0x46e>
 8003566:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8003569:	8d 50 01             	lea    0x1(%eax),%edx
 800356c:	89 55 e8             	mov    %edx,-0x18(%ebp)
 800356f:	0f b6 10             	movzbl (%eax),%edx
 8003572:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8003575:	8d 48 01             	lea    0x1(%eax),%ecx
 8003578:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 800357b:	88 94 05 04 ff ff ff 	mov    %dl,-0xfc(%ebp,%eax,1)
 8003582:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8003585:	0f b6 00             	movzbl (%eax),%eax
 8003588:	84 c0                	test   %al,%al
 800358a:	74 06                	je     8003592 <main+0x47e>
 800358c:	83 7d e4 4d          	cmpl   $0x4d,-0x1c(%ebp)
 8003590:	7e d4                	jle    8003566 <main+0x452>
 8003592:	83 ec 0c             	sub    $0xc,%esp
 8003595:	ff 75 b8             	push   -0x48(%ebp)
 8003598:	e8 25 03 00 00       	call   80038c2 <_ZNK9QTextEdit10cursorLineEv>
 800359d:	83 c4 10             	add    $0x10,%esp
 80035a0:	83 c0 01             	add    $0x1,%eax
 80035a3:	89 45 e0             	mov    %eax,-0x20(%ebp)
 80035a6:	c7 45 dc 00 00 00 00 	movl   $0x0,-0x24(%ebp)
 80035ad:	8b 4d e0             	mov    -0x20(%ebp),%ecx
 80035b0:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80035b5:	89 c8                	mov    %ecx,%eax
 80035b7:	f7 ea                	imul   %edx
 80035b9:	c1 fa 02             	sar    $0x2,%edx
 80035bc:	89 c8                	mov    %ecx,%eax
 80035be:	c1 f8 1f             	sar    $0x1f,%eax
 80035c1:	29 c2                	sub    %eax,%edx
 80035c3:	89 d0                	mov    %edx,%eax
 80035c5:	c1 e0 02             	shl    $0x2,%eax
 80035c8:	01 d0                	add    %edx,%eax
 80035ca:	01 c0                	add    %eax,%eax
 80035cc:	29 c1                	sub    %eax,%ecx
 80035ce:	89 ca                	mov    %ecx,%edx
 80035d0:	89 d0                	mov    %edx,%eax
 80035d2:	83 c0 30             	add    $0x30,%eax
 80035d5:	89 c1                	mov    %eax,%ecx
 80035d7:	8b 45 dc             	mov    -0x24(%ebp),%eax
 80035da:	8d 50 01             	lea    0x1(%eax),%edx
 80035dd:	89 55 dc             	mov    %edx,-0x24(%ebp)
 80035e0:	88 8c 05 54 ff ff ff 	mov    %cl,-0xac(%ebp,%eax,1)
 80035e7:	8b 4d e0             	mov    -0x20(%ebp),%ecx
 80035ea:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80035ef:	89 c8                	mov    %ecx,%eax
 80035f1:	f7 ea                	imul   %edx
 80035f3:	89 d0                	mov    %edx,%eax
 80035f5:	c1 f8 02             	sar    $0x2,%eax
 80035f8:	c1 f9 1f             	sar    $0x1f,%ecx
 80035fb:	89 ca                	mov    %ecx,%edx
 80035fd:	29 d0                	sub    %edx,%eax
 80035ff:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8003602:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
 8003606:	7f a5                	jg     80035ad <main+0x499>
 8003608:	eb 22                	jmp    800362c <main+0x518>
 800360a:	83 6d dc 01          	subl   $0x1,-0x24(%ebp)
 800360e:	8d 95 54 ff ff ff    	lea    -0xac(%ebp),%edx
 8003614:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8003617:	01 d0                	add    %edx,%eax
 8003619:	0f b6 10             	movzbl (%eax),%edx
 800361c:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800361f:	8d 48 01             	lea    0x1(%eax),%ecx
 8003622:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 8003625:	88 94 05 04 ff ff ff 	mov    %dl,-0xfc(%ebp,%eax,1)
 800362c:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 8003630:	7e 06                	jle    8003638 <main+0x524>
 8003632:	83 7d e4 4d          	cmpl   $0x4d,-0x1c(%ebp)
 8003636:	7e d2                	jle    800360a <main+0x4f6>
 8003638:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800363b:	8d 50 01             	lea    0x1(%eax),%edx
 800363e:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 8003641:	c6 84 05 04 ff ff ff 	movb   $0x2c,-0xfc(%ebp,%eax,1)
 8003648:	2c 
 8003649:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800364c:	8d 50 01             	lea    0x1(%eax),%edx
 800364f:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 8003652:	c6 84 05 04 ff ff ff 	movb   $0x20,-0xfc(%ebp,%eax,1)
 8003659:	20 
 800365a:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800365d:	8d 50 01             	lea    0x1(%eax),%edx
 8003660:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 8003663:	c6 84 05 04 ff ff ff 	movb   $0x43,-0xfc(%ebp,%eax,1)
 800366a:	43 
 800366b:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800366e:	8d 50 01             	lea    0x1(%eax),%edx
 8003671:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 8003674:	c6 84 05 04 ff ff ff 	movb   $0x6f,-0xfc(%ebp,%eax,1)
 800367b:	6f 
 800367c:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800367f:	8d 50 01             	lea    0x1(%eax),%edx
 8003682:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 8003685:	c6 84 05 04 ff ff ff 	movb   $0x6c,-0xfc(%ebp,%eax,1)
 800368c:	6c 
 800368d:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8003690:	8d 50 01             	lea    0x1(%eax),%edx
 8003693:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 8003696:	c6 84 05 04 ff ff ff 	movb   $0x20,-0xfc(%ebp,%eax,1)
 800369d:	20 
 800369e:	83 ec 0c             	sub    $0xc,%esp
 80036a1:	ff 75 b8             	push   -0x48(%ebp)
 80036a4:	e8 25 02 00 00       	call   80038ce <_ZNK9QTextEdit9cursorColEv>
 80036a9:	83 c4 10             	add    $0x10,%esp
 80036ac:	83 c0 01             	add    $0x1,%eax
 80036af:	89 45 d8             	mov    %eax,-0x28(%ebp)
 80036b2:	c7 45 dc 00 00 00 00 	movl   $0x0,-0x24(%ebp)
 80036b9:	8b 4d d8             	mov    -0x28(%ebp),%ecx
 80036bc:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80036c1:	89 c8                	mov    %ecx,%eax
 80036c3:	f7 ea                	imul   %edx
 80036c5:	c1 fa 02             	sar    $0x2,%edx
 80036c8:	89 c8                	mov    %ecx,%eax
 80036ca:	c1 f8 1f             	sar    $0x1f,%eax
 80036cd:	29 c2                	sub    %eax,%edx
 80036cf:	89 d0                	mov    %edx,%eax
 80036d1:	c1 e0 02             	shl    $0x2,%eax
 80036d4:	01 d0                	add    %edx,%eax
 80036d6:	01 c0                	add    %eax,%eax
 80036d8:	29 c1                	sub    %eax,%ecx
 80036da:	89 ca                	mov    %ecx,%edx
 80036dc:	89 d0                	mov    %edx,%eax
 80036de:	83 c0 30             	add    $0x30,%eax
 80036e1:	89 c1                	mov    %eax,%ecx
 80036e3:	8b 45 dc             	mov    -0x24(%ebp),%eax
 80036e6:	8d 50 01             	lea    0x1(%eax),%edx
 80036e9:	89 55 dc             	mov    %edx,-0x24(%ebp)
 80036ec:	88 8c 05 54 ff ff ff 	mov    %cl,-0xac(%ebp,%eax,1)
 80036f3:	8b 4d d8             	mov    -0x28(%ebp),%ecx
 80036f6:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80036fb:	89 c8                	mov    %ecx,%eax
 80036fd:	f7 ea                	imul   %edx
 80036ff:	89 d0                	mov    %edx,%eax
 8003701:	c1 f8 02             	sar    $0x2,%eax
 8003704:	c1 f9 1f             	sar    $0x1f,%ecx
 8003707:	89 ca                	mov    %ecx,%edx
 8003709:	29 d0                	sub    %edx,%eax
 800370b:	89 45 d8             	mov    %eax,-0x28(%ebp)
 800370e:	83 7d d8 00          	cmpl   $0x0,-0x28(%ebp)
 8003712:	7f a5                	jg     80036b9 <main+0x5a5>
 8003714:	eb 22                	jmp    8003738 <main+0x624>
 8003716:	83 6d dc 01          	subl   $0x1,-0x24(%ebp)
 800371a:	8d 95 54 ff ff ff    	lea    -0xac(%ebp),%edx
 8003720:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8003723:	01 d0                	add    %edx,%eax
 8003725:	0f b6 10             	movzbl (%eax),%edx
 8003728:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800372b:	8d 48 01             	lea    0x1(%eax),%ecx
 800372e:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 8003731:	88 94 05 04 ff ff ff 	mov    %dl,-0xfc(%ebp,%eax,1)
 8003738:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 800373c:	7e 06                	jle    8003744 <main+0x630>
 800373e:	83 7d e4 4d          	cmpl   $0x4d,-0x1c(%ebp)
 8003742:	7e d2                	jle    8003716 <main+0x602>
 8003744:	c7 45 d4 df 56 00 08 	movl   $0x80056df,-0x2c(%ebp)
 800374b:	eb 1c                	jmp    8003769 <main+0x655>
 800374d:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 8003750:	8d 50 01             	lea    0x1(%eax),%edx
 8003753:	89 55 d4             	mov    %edx,-0x2c(%ebp)
 8003756:	0f b6 10             	movzbl (%eax),%edx
 8003759:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800375c:	8d 48 01             	lea    0x1(%eax),%ecx
 800375f:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 8003762:	88 94 05 04 ff ff ff 	mov    %dl,-0xfc(%ebp,%eax,1)
 8003769:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 800376c:	0f b6 00             	movzbl (%eax),%eax
 800376f:	84 c0                	test   %al,%al
 8003771:	74 06                	je     8003779 <main+0x665>
 8003773:	83 7d e4 4d          	cmpl   $0x4d,-0x1c(%ebp)
 8003777:	7e d4                	jle    800374d <main+0x639>
 8003779:	83 ec 0c             	sub    $0xc,%esp
 800377c:	ff 75 b8             	push   -0x48(%ebp)
 800377f:	e8 32 01 00 00       	call   80038b6 <_ZN9QTextEdit8documentEv>
 8003784:	83 c4 10             	add    $0x10,%esp
 8003787:	83 ec 0c             	sub    $0xc,%esp
 800378a:	50                   	push   %eax
 800378b:	e8 22 f9 ff ff       	call   80030b2 <_ZNK13QTextDocument9lineCountEv>
 8003790:	83 c4 10             	add    $0x10,%esp
 8003793:	89 45 d0             	mov    %eax,-0x30(%ebp)
 8003796:	c7 45 dc 00 00 00 00 	movl   $0x0,-0x24(%ebp)
 800379d:	8b 4d d0             	mov    -0x30(%ebp),%ecx
 80037a0:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80037a5:	89 c8                	mov    %ecx,%eax
 80037a7:	f7 ea                	imul   %edx
 80037a9:	c1 fa 02             	sar    $0x2,%edx
 80037ac:	89 c8                	mov    %ecx,%eax
 80037ae:	c1 f8 1f             	sar    $0x1f,%eax
 80037b1:	29 c2                	sub    %eax,%edx
 80037b3:	89 d0                	mov    %edx,%eax
 80037b5:	c1 e0 02             	shl    $0x2,%eax
 80037b8:	01 d0                	add    %edx,%eax
 80037ba:	01 c0                	add    %eax,%eax
 80037bc:	29 c1                	sub    %eax,%ecx
 80037be:	89 ca                	mov    %ecx,%edx
 80037c0:	89 d0                	mov    %edx,%eax
 80037c2:	83 c0 30             	add    $0x30,%eax
 80037c5:	89 c1                	mov    %eax,%ecx
 80037c7:	8b 45 dc             	mov    -0x24(%ebp),%eax
 80037ca:	8d 50 01             	lea    0x1(%eax),%edx
 80037cd:	89 55 dc             	mov    %edx,-0x24(%ebp)
 80037d0:	88 8c 05 54 ff ff ff 	mov    %cl,-0xac(%ebp,%eax,1)
 80037d7:	8b 4d d0             	mov    -0x30(%ebp),%ecx
 80037da:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80037df:	89 c8                	mov    %ecx,%eax
 80037e1:	f7 ea                	imul   %edx
 80037e3:	89 d0                	mov    %edx,%eax
 80037e5:	c1 f8 02             	sar    $0x2,%eax
 80037e8:	c1 f9 1f             	sar    $0x1f,%ecx
 80037eb:	89 ca                	mov    %ecx,%edx
 80037ed:	29 d0                	sub    %edx,%eax
 80037ef:	89 45 d0             	mov    %eax,-0x30(%ebp)
 80037f2:	83 7d d0 00          	cmpl   $0x0,-0x30(%ebp)
 80037f6:	7f a5                	jg     800379d <main+0x689>
 80037f8:	eb 22                	jmp    800381c <main+0x708>
 80037fa:	83 6d dc 01          	subl   $0x1,-0x24(%ebp)
 80037fe:	8d 95 54 ff ff ff    	lea    -0xac(%ebp),%edx
 8003804:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8003807:	01 d0                	add    %edx,%eax
 8003809:	0f b6 10             	movzbl (%eax),%edx
 800380c:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800380f:	8d 48 01             	lea    0x1(%eax),%ecx
 8003812:	89 4d e4             	mov    %ecx,-0x1c(%ebp)
 8003815:	88 94 05 04 ff ff ff 	mov    %dl,-0xfc(%ebp,%eax,1)
 800381c:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 8003820:	7e 06                	jle    8003828 <main+0x714>
 8003822:	83 7d e4 4d          	cmpl   $0x4d,-0x1c(%ebp)
 8003826:	7e d2                	jle    80037fa <main+0x6e6>
 8003828:	8d 95 04 ff ff ff    	lea    -0xfc(%ebp),%edx
 800382e:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8003831:	01 d0                	add    %edx,%eax
 8003833:	c6 00 00             	movb   $0x0,(%eax)
 8003836:	8b 45 b4             	mov    -0x4c(%ebp),%eax
 8003839:	8d 50 08             	lea    0x8(%eax),%edx
 800383c:	8d 85 04 ff ff ff    	lea    -0xfc(%ebp),%eax
 8003842:	50                   	push   %eax
 8003843:	52                   	push   %edx
 8003844:	6a 08                	push   $0x8
 8003846:	8d 85 6c ff ff ff    	lea    -0x94(%ebp),%eax
 800384c:	50                   	push   %eax
 800384d:	e8 d8 d2 ff ff       	call   8000b2a <_ZN8QPainter8drawTextEiiPKc>
 8003852:	83 c4 10             	add    $0x10,%esp
 8003855:	e9 c9 fb ff ff       	jmp    8003423 <main+0x30f>
 800385a:	90                   	nop
 800385b:	e9 c3 fb ff ff       	jmp    8003423 <main+0x30f>
 8003860:	90                   	nop
 8003861:	e9 bd fb ff ff       	jmp    8003423 <main+0x30f>
 8003866:	8b 10                	mov    (%eax),%edx
 8003868:	83 c2 04             	add    $0x4,%edx
 800386b:	8b 12                	mov    (%edx),%edx
 800386d:	83 ec 0c             	sub    $0xc,%esp
 8003870:	50                   	push   %eax
 8003871:	ff d2                	call   *%edx
 8003873:	83 c4 10             	add    $0x10,%esp
 8003876:	83 ec 0c             	sub    $0xc,%esp
 8003879:	68 ec 56 00 08       	push   $0x80056ec
 800387e:	e8 fe 03 00 00       	call   8003c81 <printf>
 8003883:	83 c4 10             	add    $0x10,%esp
 8003886:	83 ec 0c             	sub    $0xc,%esp
 8003889:	68 17 57 00 08       	push   $0x8005717
 800388e:	e8 ee 03 00 00       	call   8003c81 <printf>
 8003893:	83 c4 10             	add    $0x10,%esp
 8003896:	83 ec 0c             	sub    $0xc,%esp
 8003899:	68 4c 53 00 08       	push   $0x800534c
 800389e:	e8 de 03 00 00       	call   8003c81 <printf>
 80038a3:	83 c4 10             	add    $0x10,%esp
 80038a6:	b8 00 00 00 00       	mov    $0x0,%eax
 80038ab:	8d 65 f8             	lea    -0x8(%ebp),%esp
 80038ae:	59                   	pop    %ecx
 80038af:	5b                   	pop    %ebx
 80038b0:	5d                   	pop    %ebp
 80038b1:	8d 61 fc             	lea    -0x4(%ecx),%esp
 80038b4:	c3                   	ret
 80038b5:	90                   	nop

080038b6 <_ZN9QTextEdit8documentEv>:
 80038b6:	55                   	push   %ebp
 80038b7:	89 e5                	mov    %esp,%ebp
 80038b9:	8b 45 08             	mov    0x8(%ebp),%eax
 80038bc:	8b 40 34             	mov    0x34(%eax),%eax
 80038bf:	5d                   	pop    %ebp
 80038c0:	c3                   	ret
 80038c1:	90                   	nop

080038c2 <_ZNK9QTextEdit10cursorLineEv>:
 80038c2:	55                   	push   %ebp
 80038c3:	89 e5                	mov    %esp,%ebp
 80038c5:	8b 45 08             	mov    0x8(%ebp),%eax
 80038c8:	8b 40 38             	mov    0x38(%eax),%eax
 80038cb:	5d                   	pop    %ebp
 80038cc:	c3                   	ret
 80038cd:	90                   	nop

080038ce <_ZNK9QTextEdit9cursorColEv>:
 80038ce:	55                   	push   %ebp
 80038cf:	89 e5                	mov    %esp,%ebp
 80038d1:	8b 45 08             	mov    0x8(%ebp),%eax
 80038d4:	8b 40 3c             	mov    0x3c(%eax),%eax
 80038d7:	5d                   	pop    %ebp
 80038d8:	c3                   	ret

080038d9 <strlen>:
 80038d9:	55                   	push   %ebp
 80038da:	89 e5                	mov    %esp,%ebp
 80038dc:	83 ec 10             	sub    $0x10,%esp
 80038df:	8b 45 08             	mov    0x8(%ebp),%eax
 80038e2:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80038e5:	eb 04                	jmp    80038eb <strlen+0x12>
 80038e7:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 80038eb:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80038ee:	0f b6 00             	movzbl (%eax),%eax
 80038f1:	84 c0                	test   %al,%al
 80038f3:	75 f2                	jne    80038e7 <strlen+0xe>
 80038f5:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80038f8:	2b 45 08             	sub    0x8(%ebp),%eax
 80038fb:	c9                   	leave
 80038fc:	c3                   	ret

080038fd <atoi>:
 80038fd:	55                   	push   %ebp
 80038fe:	89 e5                	mov    %esp,%ebp
 8003900:	83 ec 10             	sub    $0x10,%esp
 8003903:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 800390a:	eb 23                	jmp    800392f <atoi+0x32>
 800390c:	8b 55 fc             	mov    -0x4(%ebp),%edx
 800390f:	89 d0                	mov    %edx,%eax
 8003911:	c1 e0 02             	shl    $0x2,%eax
 8003914:	01 d0                	add    %edx,%eax
 8003916:	01 c0                	add    %eax,%eax
 8003918:	89 c2                	mov    %eax,%edx
 800391a:	8b 45 08             	mov    0x8(%ebp),%eax
 800391d:	0f b6 00             	movzbl (%eax),%eax
 8003920:	0f be c0             	movsbl %al,%eax
 8003923:	83 e8 30             	sub    $0x30,%eax
 8003926:	01 d0                	add    %edx,%eax
 8003928:	89 45 fc             	mov    %eax,-0x4(%ebp)
 800392b:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 800392f:	8b 45 08             	mov    0x8(%ebp),%eax
 8003932:	0f b6 00             	movzbl (%eax),%eax
 8003935:	3c 2f                	cmp    $0x2f,%al
 8003937:	7e 0a                	jle    8003943 <atoi+0x46>
 8003939:	8b 45 08             	mov    0x8(%ebp),%eax
 800393c:	0f b6 00             	movzbl (%eax),%eax
 800393f:	3c 39                	cmp    $0x39,%al
 8003941:	7e c9                	jle    800390c <atoi+0xf>
 8003943:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003946:	c9                   	leave
 8003947:	c3                   	ret

08003948 <memcpy>:
 8003948:	55                   	push   %ebp
 8003949:	89 e5                	mov    %esp,%ebp
 800394b:	83 ec 10             	sub    $0x10,%esp
 800394e:	8b 45 08             	mov    0x8(%ebp),%eax
 8003951:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003954:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003957:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800395a:	83 7d 10 0f          	cmpl   $0xf,0x10(%ebp)
 800395e:	77 49                	ja     80039a9 <memcpy+0x61>
 8003960:	eb 17                	jmp    8003979 <memcpy+0x31>
 8003962:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8003965:	8d 42 01             	lea    0x1(%edx),%eax
 8003968:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800396b:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800396e:	8d 48 01             	lea    0x1(%eax),%ecx
 8003971:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 8003974:	0f b6 12             	movzbl (%edx),%edx
 8003977:	88 10                	mov    %dl,(%eax)
 8003979:	8b 45 10             	mov    0x10(%ebp),%eax
 800397c:	8d 50 ff             	lea    -0x1(%eax),%edx
 800397f:	89 55 10             	mov    %edx,0x10(%ebp)
 8003982:	85 c0                	test   %eax,%eax
 8003984:	75 dc                	jne    8003962 <memcpy+0x1a>
 8003986:	8b 45 08             	mov    0x8(%ebp),%eax
 8003989:	e9 8e 00 00 00       	jmp    8003a1c <memcpy+0xd4>
 800398e:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8003991:	8d 42 01             	lea    0x1(%edx),%eax
 8003994:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003997:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800399a:	8d 48 01             	lea    0x1(%eax),%ecx
 800399d:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 80039a0:	0f b6 12             	movzbl (%edx),%edx
 80039a3:	88 10                	mov    %dl,(%eax)
 80039a5:	83 6d 10 01          	subl   $0x1,0x10(%ebp)
 80039a9:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80039ac:	83 e0 03             	and    $0x3,%eax
 80039af:	85 c0                	test   %eax,%eax
 80039b1:	74 06                	je     80039b9 <memcpy+0x71>
 80039b3:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 80039b7:	75 d5                	jne    800398e <memcpy+0x46>
 80039b9:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80039bc:	89 45 f4             	mov    %eax,-0xc(%ebp)
 80039bf:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80039c2:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80039c5:	eb 1a                	jmp    80039e1 <memcpy+0x99>
 80039c7:	8b 55 f0             	mov    -0x10(%ebp),%edx
 80039ca:	8d 42 04             	lea    0x4(%edx),%eax
 80039cd:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80039d0:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80039d3:	8d 48 04             	lea    0x4(%eax),%ecx
 80039d6:	89 4d f4             	mov    %ecx,-0xc(%ebp)
 80039d9:	8b 12                	mov    (%edx),%edx
 80039db:	89 10                	mov    %edx,(%eax)
 80039dd:	83 6d 10 04          	subl   $0x4,0x10(%ebp)
 80039e1:	83 7d 10 03          	cmpl   $0x3,0x10(%ebp)
 80039e5:	77 e0                	ja     80039c7 <memcpy+0x7f>
 80039e7:	8b 45 f4             	mov    -0xc(%ebp),%eax
 80039ea:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80039ed:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80039f0:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80039f3:	eb 17                	jmp    8003a0c <memcpy+0xc4>
 80039f5:	8b 55 f8             	mov    -0x8(%ebp),%edx
 80039f8:	8d 42 01             	lea    0x1(%edx),%eax
 80039fb:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80039fe:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003a01:	8d 48 01             	lea    0x1(%eax),%ecx
 8003a04:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 8003a07:	0f b6 12             	movzbl (%edx),%edx
 8003a0a:	88 10                	mov    %dl,(%eax)
 8003a0c:	8b 45 10             	mov    0x10(%ebp),%eax
 8003a0f:	8d 50 ff             	lea    -0x1(%eax),%edx
 8003a12:	89 55 10             	mov    %edx,0x10(%ebp)
 8003a15:	85 c0                	test   %eax,%eax
 8003a17:	75 dc                	jne    80039f5 <memcpy+0xad>
 8003a19:	8b 45 08             	mov    0x8(%ebp),%eax
 8003a1c:	c9                   	leave
 8003a1d:	c3                   	ret

08003a1e <memset>:
 8003a1e:	55                   	push   %ebp
 8003a1f:	89 e5                	mov    %esp,%ebp
 8003a21:	83 ec 10             	sub    $0x10,%esp
 8003a24:	8b 45 08             	mov    0x8(%ebp),%eax
 8003a27:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003a2a:	eb 0e                	jmp    8003a3a <memset+0x1c>
 8003a2c:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003a2f:	8d 50 01             	lea    0x1(%eax),%edx
 8003a32:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8003a35:	8b 55 0c             	mov    0xc(%ebp),%edx
 8003a38:	88 10                	mov    %dl,(%eax)
 8003a3a:	8b 45 10             	mov    0x10(%ebp),%eax
 8003a3d:	8d 50 ff             	lea    -0x1(%eax),%edx
 8003a40:	89 55 10             	mov    %edx,0x10(%ebp)
 8003a43:	85 c0                	test   %eax,%eax
 8003a45:	75 e5                	jne    8003a2c <memset+0xe>
 8003a47:	8b 45 08             	mov    0x8(%ebp),%eax
 8003a4a:	c9                   	leave
 8003a4b:	c3                   	ret

08003a4c <strcmp>:
 8003a4c:	55                   	push   %ebp
 8003a4d:	89 e5                	mov    %esp,%ebp
 8003a4f:	eb 08                	jmp    8003a59 <strcmp+0xd>
 8003a51:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 8003a55:	83 45 0c 01          	addl   $0x1,0xc(%ebp)
 8003a59:	8b 45 08             	mov    0x8(%ebp),%eax
 8003a5c:	0f b6 00             	movzbl (%eax),%eax
 8003a5f:	84 c0                	test   %al,%al
 8003a61:	74 10                	je     8003a73 <strcmp+0x27>
 8003a63:	8b 45 08             	mov    0x8(%ebp),%eax
 8003a66:	0f b6 10             	movzbl (%eax),%edx
 8003a69:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003a6c:	0f b6 00             	movzbl (%eax),%eax
 8003a6f:	38 c2                	cmp    %al,%dl
 8003a71:	74 de                	je     8003a51 <strcmp+0x5>
 8003a73:	8b 45 08             	mov    0x8(%ebp),%eax
 8003a76:	0f b6 00             	movzbl (%eax),%eax
 8003a79:	0f b6 d0             	movzbl %al,%edx
 8003a7c:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003a7f:	0f b6 00             	movzbl (%eax),%eax
 8003a82:	0f b6 c0             	movzbl %al,%eax
 8003a85:	29 c2                	sub    %eax,%edx
 8003a87:	89 d0                	mov    %edx,%eax
 8003a89:	5d                   	pop    %ebp
 8003a8a:	c3                   	ret

08003a8b <strncmp>:
 8003a8b:	55                   	push   %ebp
 8003a8c:	89 e5                	mov    %esp,%ebp
 8003a8e:	eb 0c                	jmp    8003a9c <strncmp+0x11>
 8003a90:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 8003a94:	83 45 0c 01          	addl   $0x1,0xc(%ebp)
 8003a98:	83 6d 10 01          	subl   $0x1,0x10(%ebp)
 8003a9c:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8003aa0:	74 1a                	je     8003abc <strncmp+0x31>
 8003aa2:	8b 45 08             	mov    0x8(%ebp),%eax
 8003aa5:	0f b6 00             	movzbl (%eax),%eax
 8003aa8:	84 c0                	test   %al,%al
 8003aaa:	74 10                	je     8003abc <strncmp+0x31>
 8003aac:	8b 45 08             	mov    0x8(%ebp),%eax
 8003aaf:	0f b6 10             	movzbl (%eax),%edx
 8003ab2:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003ab5:	0f b6 00             	movzbl (%eax),%eax
 8003ab8:	38 c2                	cmp    %al,%dl
 8003aba:	74 d4                	je     8003a90 <strncmp+0x5>
 8003abc:	83 7d 10 00          	cmpl   $0x0,0x10(%ebp)
 8003ac0:	75 07                	jne    8003ac9 <strncmp+0x3e>
 8003ac2:	ba 00 00 00 00       	mov    $0x0,%edx
 8003ac7:	eb 14                	jmp    8003add <strncmp+0x52>
 8003ac9:	8b 45 08             	mov    0x8(%ebp),%eax
 8003acc:	0f b6 00             	movzbl (%eax),%eax
 8003acf:	0f b6 d0             	movzbl %al,%edx
 8003ad2:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003ad5:	0f b6 00             	movzbl (%eax),%eax
 8003ad8:	0f b6 c0             	movzbl %al,%eax
 8003adb:	29 c2                	sub    %eax,%edx
 8003add:	89 d0                	mov    %edx,%eax
 8003adf:	5d                   	pop    %ebp
 8003ae0:	c3                   	ret

08003ae1 <memcmp>:
 8003ae1:	55                   	push   %ebp
 8003ae2:	89 e5                	mov    %esp,%ebp
 8003ae4:	83 ec 10             	sub    $0x10,%esp
 8003ae7:	8b 45 08             	mov    0x8(%ebp),%eax
 8003aea:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003aed:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003af0:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003af3:	eb 2e                	jmp    8003b23 <memcmp+0x42>
 8003af5:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003af8:	0f b6 10             	movzbl (%eax),%edx
 8003afb:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003afe:	0f b6 00             	movzbl (%eax),%eax
 8003b01:	38 c2                	cmp    %al,%dl
 8003b03:	74 16                	je     8003b1b <memcmp+0x3a>
 8003b05:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003b08:	0f b6 00             	movzbl (%eax),%eax
 8003b0b:	0f b6 d0             	movzbl %al,%edx
 8003b0e:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003b11:	0f b6 00             	movzbl (%eax),%eax
 8003b14:	0f b6 c0             	movzbl %al,%eax
 8003b17:	29 c2                	sub    %eax,%edx
 8003b19:	eb 1a                	jmp    8003b35 <memcmp+0x54>
 8003b1b:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 8003b1f:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
 8003b23:	8b 45 10             	mov    0x10(%ebp),%eax
 8003b26:	8d 50 ff             	lea    -0x1(%eax),%edx
 8003b29:	89 55 10             	mov    %edx,0x10(%ebp)
 8003b2c:	85 c0                	test   %eax,%eax
 8003b2e:	75 c5                	jne    8003af5 <memcmp+0x14>
 8003b30:	ba 00 00 00 00       	mov    $0x0,%edx
 8003b35:	89 d0                	mov    %edx,%eax
 8003b37:	c9                   	leave
 8003b38:	c3                   	ret

08003b39 <strcpy>:
 8003b39:	55                   	push   %ebp
 8003b3a:	89 e5                	mov    %esp,%ebp
 8003b3c:	83 ec 10             	sub    $0x10,%esp
 8003b3f:	8b 45 08             	mov    0x8(%ebp),%eax
 8003b42:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003b45:	90                   	nop
 8003b46:	8b 55 0c             	mov    0xc(%ebp),%edx
 8003b49:	8d 42 01             	lea    0x1(%edx),%eax
 8003b4c:	89 45 0c             	mov    %eax,0xc(%ebp)
 8003b4f:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003b52:	8d 48 01             	lea    0x1(%eax),%ecx
 8003b55:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 8003b58:	0f b6 12             	movzbl (%edx),%edx
 8003b5b:	88 10                	mov    %dl,(%eax)
 8003b5d:	0f b6 00             	movzbl (%eax),%eax
 8003b60:	84 c0                	test   %al,%al
 8003b62:	75 e2                	jne    8003b46 <strcpy+0xd>
 8003b64:	8b 45 08             	mov    0x8(%ebp),%eax
 8003b67:	c9                   	leave
 8003b68:	c3                   	ret

08003b69 <itoa>:
 8003b69:	55                   	push   %ebp
 8003b6a:	89 e5                	mov    %esp,%ebp
 8003b6c:	83 ec 20             	sub    $0x20,%esp
 8003b6f:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8003b76:	c7 45 f8 00 00 00 00 	movl   $0x0,-0x8(%ebp)
 8003b7d:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 8003b81:	75 26                	jne    8003ba9 <itoa+0x40>
 8003b83:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003b86:	8d 50 01             	lea    0x1(%eax),%edx
 8003b89:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8003b8c:	89 c2                	mov    %eax,%edx
 8003b8e:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003b91:	01 d0                	add    %edx,%eax
 8003b93:	c6 00 30             	movb   $0x30,(%eax)
 8003b96:	8b 55 fc             	mov    -0x4(%ebp),%edx
 8003b99:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003b9c:	01 d0                	add    %edx,%eax
 8003b9e:	c6 00 00             	movb   $0x0,(%eax)
 8003ba1:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003ba4:	e9 d6 00 00 00       	jmp    8003c7f <itoa+0x116>
 8003ba9:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 8003bad:	79 50                	jns    8003bff <itoa+0x96>
 8003baf:	83 7d 10 0a          	cmpl   $0xa,0x10(%ebp)
 8003bb3:	75 4a                	jne    8003bff <itoa+0x96>
 8003bb5:	c7 45 f8 01 00 00 00 	movl   $0x1,-0x8(%ebp)
 8003bbc:	f7 5d 08             	negl   0x8(%ebp)
 8003bbf:	eb 3e                	jmp    8003bff <itoa+0x96>
 8003bc1:	8b 45 08             	mov    0x8(%ebp),%eax
 8003bc4:	99                   	cltd
 8003bc5:	f7 7d 10             	idivl  0x10(%ebp)
 8003bc8:	89 55 e8             	mov    %edx,-0x18(%ebp)
 8003bcb:	83 7d e8 09          	cmpl   $0x9,-0x18(%ebp)
 8003bcf:	7e 0a                	jle    8003bdb <itoa+0x72>
 8003bd1:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8003bd4:	83 c0 57             	add    $0x57,%eax
 8003bd7:	89 c1                	mov    %eax,%ecx
 8003bd9:	eb 08                	jmp    8003be3 <itoa+0x7a>
 8003bdb:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8003bde:	83 c0 30             	add    $0x30,%eax
 8003be1:	89 c1                	mov    %eax,%ecx
 8003be3:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003be6:	8d 50 01             	lea    0x1(%eax),%edx
 8003be9:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8003bec:	89 c2                	mov    %eax,%edx
 8003bee:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003bf1:	01 d0                	add    %edx,%eax
 8003bf3:	88 08                	mov    %cl,(%eax)
 8003bf5:	8b 45 08             	mov    0x8(%ebp),%eax
 8003bf8:	99                   	cltd
 8003bf9:	f7 7d 10             	idivl  0x10(%ebp)
 8003bfc:	89 45 08             	mov    %eax,0x8(%ebp)
 8003bff:	83 7d 08 00          	cmpl   $0x0,0x8(%ebp)
 8003c03:	75 bc                	jne    8003bc1 <itoa+0x58>
 8003c05:	83 7d f8 00          	cmpl   $0x0,-0x8(%ebp)
 8003c09:	74 13                	je     8003c1e <itoa+0xb5>
 8003c0b:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003c0e:	8d 50 01             	lea    0x1(%eax),%edx
 8003c11:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8003c14:	89 c2                	mov    %eax,%edx
 8003c16:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003c19:	01 d0                	add    %edx,%eax
 8003c1b:	c6 00 2d             	movb   $0x2d,(%eax)
 8003c1e:	8b 55 fc             	mov    -0x4(%ebp),%edx
 8003c21:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003c24:	01 d0                	add    %edx,%eax
 8003c26:	c6 00 00             	movb   $0x0,(%eax)
 8003c29:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8003c30:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003c33:	83 e8 01             	sub    $0x1,%eax
 8003c36:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8003c39:	eb 39                	jmp    8003c74 <itoa+0x10b>
 8003c3b:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8003c3e:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003c41:	01 d0                	add    %edx,%eax
 8003c43:	0f b6 00             	movzbl (%eax),%eax
 8003c46:	88 45 ef             	mov    %al,-0x11(%ebp)
 8003c49:	8b 55 f0             	mov    -0x10(%ebp),%edx
 8003c4c:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003c4f:	01 d0                	add    %edx,%eax
 8003c51:	8b 4d f4             	mov    -0xc(%ebp),%ecx
 8003c54:	8b 55 0c             	mov    0xc(%ebp),%edx
 8003c57:	01 ca                	add    %ecx,%edx
 8003c59:	0f b6 00             	movzbl (%eax),%eax
 8003c5c:	88 02                	mov    %al,(%edx)
 8003c5e:	8b 55 f0             	mov    -0x10(%ebp),%edx
 8003c61:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003c64:	01 c2                	add    %eax,%edx
 8003c66:	0f b6 45 ef          	movzbl -0x11(%ebp),%eax
 8003c6a:	88 02                	mov    %al,(%edx)
 8003c6c:	83 45 f4 01          	addl   $0x1,-0xc(%ebp)
 8003c70:	83 6d f0 01          	subl   $0x1,-0x10(%ebp)
 8003c74:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8003c77:	3b 45 f0             	cmp    -0x10(%ebp),%eax
 8003c7a:	7c bf                	jl     8003c3b <itoa+0xd2>
 8003c7c:	8b 45 0c             	mov    0xc(%ebp),%eax
 8003c7f:	c9                   	leave
 8003c80:	c3                   	ret

08003c81 <printf>:
 8003c81:	55                   	push   %ebp
 8003c82:	89 e5                	mov    %esp,%ebp
 8003c84:	81 ec 58 04 00 00    	sub    $0x458,%esp
 8003c8a:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8003c91:	8d 45 0c             	lea    0xc(%ebp),%eax
 8003c94:	89 85 d4 fb ff ff    	mov    %eax,-0x42c(%ebp)
 8003c9a:	e9 d9 01 00 00       	jmp    8003e78 <printf+0x1f7>
 8003c9f:	8b 45 08             	mov    0x8(%ebp),%eax
 8003ca2:	0f b6 00             	movzbl (%eax),%eax
 8003ca5:	3c 25                	cmp    $0x25,%al
 8003ca7:	0f 85 a6 01 00 00    	jne    8003e53 <printf+0x1d2>
 8003cad:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 8003cb1:	8b 45 08             	mov    0x8(%ebp),%eax
 8003cb4:	0f b6 00             	movzbl (%eax),%eax
 8003cb7:	84 c0                	test   %al,%al
 8003cb9:	0f 84 c9 01 00 00    	je     8003e88 <printf+0x207>
 8003cbf:	8b 45 08             	mov    0x8(%ebp),%eax
 8003cc2:	0f b6 00             	movzbl (%eax),%eax
 8003cc5:	3c 64                	cmp    $0x64,%al
 8003cc7:	74 0a                	je     8003cd3 <printf+0x52>
 8003cc9:	8b 45 08             	mov    0x8(%ebp),%eax
 8003ccc:	0f b6 00             	movzbl (%eax),%eax
 8003ccf:	3c 75                	cmp    $0x75,%al
 8003cd1:	75 64                	jne    8003d37 <printf+0xb6>
 8003cd3:	8b 85 d4 fb ff ff    	mov    -0x42c(%ebp),%eax
 8003cd9:	8d 50 04             	lea    0x4(%eax),%edx
 8003cdc:	89 95 d4 fb ff ff    	mov    %edx,-0x42c(%ebp)
 8003ce2:	8b 00                	mov    (%eax),%eax
 8003ce4:	89 45 dc             	mov    %eax,-0x24(%ebp)
 8003ce7:	6a 0a                	push   $0xa
 8003ce9:	8d 85 b4 fb ff ff    	lea    -0x44c(%ebp),%eax
 8003cef:	50                   	push   %eax
 8003cf0:	ff 75 dc             	push   -0x24(%ebp)
 8003cf3:	e8 71 fe ff ff       	call   8003b69 <itoa>
 8003cf8:	83 c4 0c             	add    $0xc,%esp
 8003cfb:	8d 85 b4 fb ff ff    	lea    -0x44c(%ebp),%eax
 8003d01:	50                   	push   %eax
 8003d02:	e8 d2 fb ff ff       	call   80038d9 <strlen>
 8003d07:	83 c4 04             	add    $0x4,%esp
 8003d0a:	89 45 d8             	mov    %eax,-0x28(%ebp)
 8003d0d:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8003d10:	8d 8d d8 fb ff ff    	lea    -0x428(%ebp),%ecx
 8003d16:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8003d19:	01 ca                	add    %ecx,%edx
 8003d1b:	50                   	push   %eax
 8003d1c:	8d 85 b4 fb ff ff    	lea    -0x44c(%ebp),%eax
 8003d22:	50                   	push   %eax
 8003d23:	52                   	push   %edx
 8003d24:	e8 1f fc ff ff       	call   8003948 <memcpy>
 8003d29:	83 c4 0c             	add    $0xc,%esp
 8003d2c:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8003d2f:	01 45 f4             	add    %eax,-0xc(%ebp)
 8003d32:	e9 16 01 00 00       	jmp    8003e4d <printf+0x1cc>
 8003d37:	8b 45 08             	mov    0x8(%ebp),%eax
 8003d3a:	0f b6 00             	movzbl (%eax),%eax
 8003d3d:	3c 78                	cmp    $0x78,%al
 8003d3f:	74 0a                	je     8003d4b <printf+0xca>
 8003d41:	8b 45 08             	mov    0x8(%ebp),%eax
 8003d44:	0f b6 00             	movzbl (%eax),%eax
 8003d47:	3c 58                	cmp    $0x58,%al
 8003d49:	75 64                	jne    8003daf <printf+0x12e>
 8003d4b:	8b 85 d4 fb ff ff    	mov    -0x42c(%ebp),%eax
 8003d51:	8d 50 04             	lea    0x4(%eax),%edx
 8003d54:	89 95 d4 fb ff ff    	mov    %edx,-0x42c(%ebp)
 8003d5a:	8b 00                	mov    (%eax),%eax
 8003d5c:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 8003d5f:	6a 10                	push   $0x10
 8003d61:	8d 85 b4 fb ff ff    	lea    -0x44c(%ebp),%eax
 8003d67:	50                   	push   %eax
 8003d68:	ff 75 e4             	push   -0x1c(%ebp)
 8003d6b:	e8 f9 fd ff ff       	call   8003b69 <itoa>
 8003d70:	83 c4 0c             	add    $0xc,%esp
 8003d73:	8d 85 b4 fb ff ff    	lea    -0x44c(%ebp),%eax
 8003d79:	50                   	push   %eax
 8003d7a:	e8 5a fb ff ff       	call   80038d9 <strlen>
 8003d7f:	83 c4 04             	add    $0x4,%esp
 8003d82:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8003d85:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8003d88:	8d 8d d8 fb ff ff    	lea    -0x428(%ebp),%ecx
 8003d8e:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8003d91:	01 ca                	add    %ecx,%edx
 8003d93:	50                   	push   %eax
 8003d94:	8d 85 b4 fb ff ff    	lea    -0x44c(%ebp),%eax
 8003d9a:	50                   	push   %eax
 8003d9b:	52                   	push   %edx
 8003d9c:	e8 a7 fb ff ff       	call   8003948 <memcpy>
 8003da1:	83 c4 0c             	add    $0xc,%esp
 8003da4:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8003da7:	01 45 f4             	add    %eax,-0xc(%ebp)
 8003daa:	e9 9e 00 00 00       	jmp    8003e4d <printf+0x1cc>
 8003daf:	8b 45 08             	mov    0x8(%ebp),%eax
 8003db2:	0f b6 00             	movzbl (%eax),%eax
 8003db5:	3c 73                	cmp    $0x73,%al
 8003db7:	75 45                	jne    8003dfe <printf+0x17d>
 8003db9:	8b 85 d4 fb ff ff    	mov    -0x42c(%ebp),%eax
 8003dbf:	8d 50 04             	lea    0x4(%eax),%edx
 8003dc2:	89 95 d4 fb ff ff    	mov    %edx,-0x42c(%ebp)
 8003dc8:	8b 00                	mov    (%eax),%eax
 8003dca:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8003dcd:	ff 75 ec             	push   -0x14(%ebp)
 8003dd0:	e8 04 fb ff ff       	call   80038d9 <strlen>
 8003dd5:	83 c4 04             	add    $0x4,%esp
 8003dd8:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8003ddb:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8003dde:	8d 8d d8 fb ff ff    	lea    -0x428(%ebp),%ecx
 8003de4:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8003de7:	01 ca                	add    %ecx,%edx
 8003de9:	50                   	push   %eax
 8003dea:	ff 75 ec             	push   -0x14(%ebp)
 8003ded:	52                   	push   %edx
 8003dee:	e8 55 fb ff ff       	call   8003948 <memcpy>
 8003df3:	83 c4 0c             	add    $0xc,%esp
 8003df6:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8003df9:	01 45 f4             	add    %eax,-0xc(%ebp)
 8003dfc:	eb 4f                	jmp    8003e4d <printf+0x1cc>
 8003dfe:	8b 45 08             	mov    0x8(%ebp),%eax
 8003e01:	0f b6 00             	movzbl (%eax),%eax
 8003e04:	3c 63                	cmp    $0x63,%al
 8003e06:	75 2a                	jne    8003e32 <printf+0x1b1>
 8003e08:	8b 85 d4 fb ff ff    	mov    -0x42c(%ebp),%eax
 8003e0e:	8d 50 04             	lea    0x4(%eax),%edx
 8003e11:	89 95 d4 fb ff ff    	mov    %edx,-0x42c(%ebp)
 8003e17:	8b 00                	mov    (%eax),%eax
 8003e19:	88 45 f3             	mov    %al,-0xd(%ebp)
 8003e1c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8003e1f:	8d 50 01             	lea    0x1(%eax),%edx
 8003e22:	89 55 f4             	mov    %edx,-0xc(%ebp)
 8003e25:	0f b6 55 f3          	movzbl -0xd(%ebp),%edx
 8003e29:	88 94 05 d8 fb ff ff 	mov    %dl,-0x428(%ebp,%eax,1)
 8003e30:	eb 1b                	jmp    8003e4d <printf+0x1cc>
 8003e32:	8b 45 08             	mov    0x8(%ebp),%eax
 8003e35:	0f b6 00             	movzbl (%eax),%eax
 8003e38:	3c 25                	cmp    $0x25,%al
 8003e3a:	75 11                	jne    8003e4d <printf+0x1cc>
 8003e3c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8003e3f:	8d 50 01             	lea    0x1(%eax),%edx
 8003e42:	89 55 f4             	mov    %edx,-0xc(%ebp)
 8003e45:	c6 84 05 d8 fb ff ff 	movb   $0x25,-0x428(%ebp,%eax,1)
 8003e4c:	25 
 8003e4d:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 8003e51:	eb 1c                	jmp    8003e6f <printf+0x1ee>
 8003e53:	8b 55 08             	mov    0x8(%ebp),%edx
 8003e56:	8d 42 01             	lea    0x1(%edx),%eax
 8003e59:	89 45 08             	mov    %eax,0x8(%ebp)
 8003e5c:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8003e5f:	8d 48 01             	lea    0x1(%eax),%ecx
 8003e62:	89 4d f4             	mov    %ecx,-0xc(%ebp)
 8003e65:	0f b6 12             	movzbl (%edx),%edx
 8003e68:	88 94 05 d8 fb ff ff 	mov    %dl,-0x428(%ebp,%eax,1)
 8003e6f:	81 7d f4 f1 03 00 00 	cmpl   $0x3f1,-0xc(%ebp)
 8003e76:	7f 13                	jg     8003e8b <printf+0x20a>
 8003e78:	8b 45 08             	mov    0x8(%ebp),%eax
 8003e7b:	0f b6 00             	movzbl (%eax),%eax
 8003e7e:	84 c0                	test   %al,%al
 8003e80:	0f 85 19 fe ff ff    	jne    8003c9f <printf+0x1e>
 8003e86:	eb 04                	jmp    8003e8c <printf+0x20b>
 8003e88:	90                   	nop
 8003e89:	eb 01                	jmp    8003e8c <printf+0x20b>
 8003e8b:	90                   	nop
 8003e8c:	8d 95 d8 fb ff ff    	lea    -0x428(%ebp),%edx
 8003e92:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8003e95:	01 d0                	add    %edx,%eax
 8003e97:	c6 00 00             	movb   $0x0,(%eax)
 8003e9a:	83 ec 04             	sub    $0x4,%esp
 8003e9d:	ff 75 f4             	push   -0xc(%ebp)
 8003ea0:	8d 85 d8 fb ff ff    	lea    -0x428(%ebp),%eax
 8003ea6:	50                   	push   %eax
 8003ea7:	6a 01                	push   $0x1
 8003ea9:	e8 08 00 00 00       	call   8003eb6 <write>
 8003eae:	83 c4 10             	add    $0x10,%esp
 8003eb1:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8003eb4:	c9                   	leave
 8003eb5:	c3                   	ret

08003eb6 <write>:
 8003eb6:	55                   	push   %ebp
 8003eb7:	89 e5                	mov    %esp,%ebp
 8003eb9:	53                   	push   %ebx
 8003eba:	83 ec 10             	sub    $0x10,%esp
 8003ebd:	b8 0b 00 00 00       	mov    $0xb,%eax
 8003ec2:	8b 5d 08             	mov    0x8(%ebp),%ebx
 8003ec5:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8003ec8:	8b 55 10             	mov    0x10(%ebp),%edx
 8003ecb:	cd 80                	int    $0x80
 8003ecd:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003ed0:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003ed3:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003ed6:	c9                   	leave
 8003ed7:	c3                   	ret

08003ed8 <fork>:
 8003ed8:	55                   	push   %ebp
 8003ed9:	89 e5                	mov    %esp,%ebp
 8003edb:	83 ec 10             	sub    $0x10,%esp
 8003ede:	b8 0c 00 00 00       	mov    $0xc,%eax
 8003ee3:	cd 80                	int    $0x80
 8003ee5:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003ee8:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003eeb:	c9                   	leave
 8003eec:	c3                   	ret

08003eed <exit>:
 8003eed:	55                   	push   %ebp
 8003eee:	89 e5                	mov    %esp,%ebp
 8003ef0:	53                   	push   %ebx
 8003ef1:	b8 02 00 00 00       	mov    $0x2,%eax
 8003ef6:	8b 55 08             	mov    0x8(%ebp),%edx
 8003ef9:	89 d3                	mov    %edx,%ebx
 8003efb:	cd 80                	int    $0x80
 8003efd:	f4                   	hlt
 8003efe:	eb fd                	jmp    8003efd <exit+0x10>

08003f00 <yield>:
 8003f00:	55                   	push   %ebp
 8003f01:	89 e5                	mov    %esp,%ebp
 8003f03:	b8 03 00 00 00       	mov    $0x3,%eax
 8003f08:	cd 80                	int    $0x80
 8003f0a:	90                   	nop
 8003f0b:	5d                   	pop    %ebp
 8003f0c:	c3                   	ret

08003f0d <open>:
 8003f0d:	55                   	push   %ebp
 8003f0e:	89 e5                	mov    %esp,%ebp
 8003f10:	53                   	push   %ebx
 8003f11:	83 ec 10             	sub    $0x10,%esp
 8003f14:	b8 14 00 00 00       	mov    $0x14,%eax
 8003f19:	8b 55 08             	mov    0x8(%ebp),%edx
 8003f1c:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8003f1f:	89 d3                	mov    %edx,%ebx
 8003f21:	cd 80                	int    $0x80
 8003f23:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003f26:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003f29:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003f2c:	c9                   	leave
 8003f2d:	c3                   	ret

08003f2e <close>:
 8003f2e:	55                   	push   %ebp
 8003f2f:	89 e5                	mov    %esp,%ebp
 8003f31:	53                   	push   %ebx
 8003f32:	83 ec 10             	sub    $0x10,%esp
 8003f35:	b8 15 00 00 00       	mov    $0x15,%eax
 8003f3a:	8b 55 08             	mov    0x8(%ebp),%edx
 8003f3d:	89 d3                	mov    %edx,%ebx
 8003f3f:	cd 80                	int    $0x80
 8003f41:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003f44:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003f47:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003f4a:	c9                   	leave
 8003f4b:	c3                   	ret

08003f4c <read>:
 8003f4c:	55                   	push   %ebp
 8003f4d:	89 e5                	mov    %esp,%ebp
 8003f4f:	53                   	push   %ebx
 8003f50:	83 ec 10             	sub    $0x10,%esp
 8003f53:	b8 16 00 00 00       	mov    $0x16,%eax
 8003f58:	8b 5d 08             	mov    0x8(%ebp),%ebx
 8003f5b:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8003f5e:	8b 55 10             	mov    0x10(%ebp),%edx
 8003f61:	cd 80                	int    $0x80
 8003f63:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003f66:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003f69:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003f6c:	c9                   	leave
 8003f6d:	c3                   	ret

08003f6e <lseek>:
 8003f6e:	55                   	push   %ebp
 8003f6f:	89 e5                	mov    %esp,%ebp
 8003f71:	53                   	push   %ebx
 8003f72:	83 ec 10             	sub    $0x10,%esp
 8003f75:	b8 17 00 00 00       	mov    $0x17,%eax
 8003f7a:	8b 5d 08             	mov    0x8(%ebp),%ebx
 8003f7d:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8003f80:	8b 55 10             	mov    0x10(%ebp),%edx
 8003f83:	cd 80                	int    $0x80
 8003f85:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003f88:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003f8b:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003f8e:	c9                   	leave
 8003f8f:	c3                   	ret

08003f90 <net_ping>:
 8003f90:	55                   	push   %ebp
 8003f91:	89 e5                	mov    %esp,%ebp
 8003f93:	53                   	push   %ebx
 8003f94:	83 ec 10             	sub    $0x10,%esp
 8003f97:	b8 1e 00 00 00       	mov    $0x1e,%eax
 8003f9c:	8b 55 08             	mov    0x8(%ebp),%edx
 8003f9f:	b9 00 00 00 00       	mov    $0x0,%ecx
 8003fa4:	89 d3                	mov    %edx,%ebx
 8003fa6:	cd 80                	int    $0x80
 8003fa8:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003fab:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003fae:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003fb1:	c9                   	leave
 8003fb2:	c3                   	ret

08003fb3 <net_ping_dev>:
 8003fb3:	55                   	push   %ebp
 8003fb4:	89 e5                	mov    %esp,%ebp
 8003fb6:	53                   	push   %ebx
 8003fb7:	83 ec 10             	sub    $0x10,%esp
 8003fba:	b8 1e 00 00 00       	mov    $0x1e,%eax
 8003fbf:	8b 55 08             	mov    0x8(%ebp),%edx
 8003fc2:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8003fc5:	89 d3                	mov    %edx,%ebx
 8003fc7:	cd 80                	int    $0x80
 8003fc9:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8003fcc:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8003fcf:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8003fd2:	c9                   	leave
 8003fd3:	c3                   	ret

08003fd4 <net_ifconfig>:
 8003fd4:	55                   	push   %ebp
 8003fd5:	89 e5                	mov    %esp,%ebp
 8003fd7:	83 ec 10             	sub    $0x10,%esp
 8003fda:	b8 1f 00 00 00       	mov    $0x1f,%eax
 8003fdf:	cd 80                	int    $0x80
 8003fe1:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003fe4:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003fe7:	c9                   	leave
 8003fe8:	c3                   	ret

08003fe9 <lspci>:
 8003fe9:	55                   	push   %ebp
 8003fea:	89 e5                	mov    %esp,%ebp
 8003fec:	83 ec 10             	sub    $0x10,%esp
 8003fef:	b8 2a 00 00 00       	mov    $0x2a,%eax
 8003ff4:	cd 80                	int    $0x80
 8003ff6:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8003ff9:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8003ffc:	c9                   	leave
 8003ffd:	c3                   	ret

08003ffe <rtl8139_init_user>:
 8003ffe:	55                   	push   %ebp
 8003fff:	89 e5                	mov    %esp,%ebp
 8004001:	83 ec 10             	sub    $0x10,%esp
 8004004:	b8 2b 00 00 00       	mov    $0x2b,%eax
 8004009:	cd 80                	int    $0x80
 800400b:	89 45 fc             	mov    %eax,-0x4(%ebp)
 800400e:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004011:	c9                   	leave
 8004012:	c3                   	ret

08004013 <e1000_init_user>:
 8004013:	55                   	push   %ebp
 8004014:	89 e5                	mov    %esp,%ebp
 8004016:	53                   	push   %ebx
 8004017:	83 ec 10             	sub    $0x10,%esp
 800401a:	b8 2c 00 00 00       	mov    $0x2c,%eax
 800401f:	8b 55 08             	mov    0x8(%ebp),%edx
 8004022:	89 d3                	mov    %edx,%ebx
 8004024:	cd 80                	int    $0x80
 8004026:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8004029:	8b 45 f8             	mov    -0x8(%ebp),%eax
 800402c:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 800402f:	c9                   	leave
 8004030:	c3                   	ret

08004031 <net_send_udp>:
 8004031:	55                   	push   %ebp
 8004032:	89 e5                	mov    %esp,%ebp
 8004034:	56                   	push   %esi
 8004035:	53                   	push   %ebx
 8004036:	83 ec 10             	sub    $0x10,%esp
 8004039:	b8 2d 00 00 00       	mov    $0x2d,%eax
 800403e:	8b 5d 08             	mov    0x8(%ebp),%ebx
 8004041:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8004044:	8b 55 10             	mov    0x10(%ebp),%edx
 8004047:	8b 75 14             	mov    0x14(%ebp),%esi
 800404a:	cd 80                	int    $0x80
 800404c:	89 45 f4             	mov    %eax,-0xc(%ebp)
 800404f:	8b 45 f4             	mov    -0xc(%ebp),%eax
 8004052:	83 c4 10             	add    $0x10,%esp
 8004055:	5b                   	pop    %ebx
 8004056:	5e                   	pop    %esi
 8004057:	5d                   	pop    %ebp
 8004058:	c3                   	ret

08004059 <net_set_device>:
 8004059:	55                   	push   %ebp
 800405a:	89 e5                	mov    %esp,%ebp
 800405c:	53                   	push   %ebx
 800405d:	83 ec 10             	sub    $0x10,%esp
 8004060:	b8 2e 00 00 00       	mov    $0x2e,%eax
 8004065:	8b 55 08             	mov    0x8(%ebp),%edx
 8004068:	89 d3                	mov    %edx,%ebx
 800406a:	cd 80                	int    $0x80
 800406c:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800406f:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004072:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004075:	c9                   	leave
 8004076:	c3                   	ret

08004077 <net_poll_rx>:
 8004077:	55                   	push   %ebp
 8004078:	89 e5                	mov    %esp,%ebp
 800407a:	83 ec 10             	sub    $0x10,%esp
 800407d:	b8 2f 00 00 00       	mov    $0x2f,%eax
 8004082:	cd 80                	int    $0x80
 8004084:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8004087:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800408a:	c9                   	leave
 800408b:	c3                   	ret

0800408c <net_dump_regs>:
 800408c:	55                   	push   %ebp
 800408d:	89 e5                	mov    %esp,%ebp
 800408f:	53                   	push   %ebx
 8004090:	83 ec 10             	sub    $0x10,%esp
 8004093:	b8 30 00 00 00       	mov    $0x30,%eax
 8004098:	8b 55 08             	mov    0x8(%ebp),%edx
 800409b:	89 d3                	mov    %edx,%ebx
 800409d:	cd 80                	int    $0x80
 800409f:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80040a2:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80040a5:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80040a8:	c9                   	leave
 80040a9:	c3                   	ret

080040aa <net_arp>:
 80040aa:	55                   	push   %ebp
 80040ab:	89 e5                	mov    %esp,%ebp
 80040ad:	53                   	push   %ebx
 80040ae:	83 ec 10             	sub    $0x10,%esp
 80040b1:	b8 31 00 00 00       	mov    $0x31,%eax
 80040b6:	8b 55 08             	mov    0x8(%ebp),%edx
 80040b9:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 80040bc:	89 d3                	mov    %edx,%ebx
 80040be:	cd 80                	int    $0x80
 80040c0:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80040c3:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80040c6:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80040c9:	c9                   	leave
 80040ca:	c3                   	ret

080040cb <net_dump_rx_regs>:
 80040cb:	55                   	push   %ebp
 80040cc:	89 e5                	mov    %esp,%ebp
 80040ce:	53                   	push   %ebx
 80040cf:	83 ec 10             	sub    $0x10,%esp
 80040d2:	b8 32 00 00 00       	mov    $0x32,%eax
 80040d7:	8b 55 08             	mov    0x8(%ebp),%edx
 80040da:	89 d3                	mov    %edx,%ebx
 80040dc:	cd 80                	int    $0x80
 80040de:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80040e1:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80040e4:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80040e7:	c9                   	leave
 80040e8:	c3                   	ret

080040e9 <net_ifup>:
 80040e9:	55                   	push   %ebp
 80040ea:	89 e5                	mov    %esp,%ebp
 80040ec:	53                   	push   %ebx
 80040ed:	83 ec 10             	sub    $0x10,%esp
 80040f0:	b8 33 00 00 00       	mov    $0x33,%eax
 80040f5:	8b 55 08             	mov    0x8(%ebp),%edx
 80040f8:	89 d3                	mov    %edx,%ebx
 80040fa:	cd 80                	int    $0x80
 80040fc:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80040ff:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004102:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004105:	c9                   	leave
 8004106:	c3                   	ret

08004107 <net_bind_udp>:
 8004107:	55                   	push   %ebp
 8004108:	89 e5                	mov    %esp,%ebp
 800410a:	53                   	push   %ebx
 800410b:	83 ec 10             	sub    $0x10,%esp
 800410e:	b8 34 00 00 00       	mov    $0x34,%eax
 8004113:	8b 55 08             	mov    0x8(%ebp),%edx
 8004116:	89 d3                	mov    %edx,%ebx
 8004118:	cd 80                	int    $0x80
 800411a:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800411d:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004120:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004123:	c9                   	leave
 8004124:	c3                   	ret

08004125 <net_recv_udp>:
 8004125:	55                   	push   %ebp
 8004126:	89 e5                	mov    %esp,%ebp
 8004128:	53                   	push   %ebx
 8004129:	83 ec 10             	sub    $0x10,%esp
 800412c:	b8 35 00 00 00       	mov    $0x35,%eax
 8004131:	8b 55 08             	mov    0x8(%ebp),%edx
 8004134:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8004137:	89 d3                	mov    %edx,%ebx
 8004139:	cd 80                	int    $0x80
 800413b:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800413e:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004141:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004144:	c9                   	leave
 8004145:	c3                   	ret

08004146 <wifi_init>:
 8004146:	55                   	push   %ebp
 8004147:	89 e5                	mov    %esp,%ebp
 8004149:	83 ec 10             	sub    $0x10,%esp
 800414c:	b8 24 00 00 00       	mov    $0x24,%eax
 8004151:	cd 80                	int    $0x80
 8004153:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8004156:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004159:	c9                   	leave
 800415a:	c3                   	ret

0800415b <wifi_scan>:
 800415b:	55                   	push   %ebp
 800415c:	89 e5                	mov    %esp,%ebp
 800415e:	83 ec 10             	sub    $0x10,%esp
 8004161:	b8 20 00 00 00       	mov    $0x20,%eax
 8004166:	cd 80                	int    $0x80
 8004168:	89 45 fc             	mov    %eax,-0x4(%ebp)
 800416b:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800416e:	c9                   	leave
 800416f:	c3                   	ret

08004170 <wifi_connect>:
 8004170:	55                   	push   %ebp
 8004171:	89 e5                	mov    %esp,%ebp
 8004173:	53                   	push   %ebx
 8004174:	83 ec 10             	sub    $0x10,%esp
 8004177:	b8 21 00 00 00       	mov    $0x21,%eax
 800417c:	8b 55 08             	mov    0x8(%ebp),%edx
 800417f:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 8004182:	89 d3                	mov    %edx,%ebx
 8004184:	cd 80                	int    $0x80
 8004186:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8004189:	8b 45 f8             	mov    -0x8(%ebp),%eax
 800418c:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 800418f:	c9                   	leave
 8004190:	c3                   	ret

08004191 <wifi_disconnect>:
 8004191:	55                   	push   %ebp
 8004192:	89 e5                	mov    %esp,%ebp
 8004194:	83 ec 10             	sub    $0x10,%esp
 8004197:	b8 22 00 00 00       	mov    $0x22,%eax
 800419c:	cd 80                	int    $0x80
 800419e:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80041a1:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80041a4:	c9                   	leave
 80041a5:	c3                   	ret

080041a6 <wifi_status>:
 80041a6:	55                   	push   %ebp
 80041a7:	89 e5                	mov    %esp,%ebp
 80041a9:	b8 23 00 00 00       	mov    $0x23,%eax
 80041ae:	cd 80                	int    $0x80
 80041b0:	90                   	nop
 80041b1:	5d                   	pop    %ebp
 80041b2:	c3                   	ret

080041b3 <wifi_fw_load_begin>:
 80041b3:	55                   	push   %ebp
 80041b4:	89 e5                	mov    %esp,%ebp
 80041b6:	53                   	push   %ebx
 80041b7:	83 ec 10             	sub    $0x10,%esp
 80041ba:	b8 25 00 00 00       	mov    $0x25,%eax
 80041bf:	8b 55 08             	mov    0x8(%ebp),%edx
 80041c2:	89 d3                	mov    %edx,%ebx
 80041c4:	cd 80                	int    $0x80
 80041c6:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80041c9:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80041cc:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80041cf:	c9                   	leave
 80041d0:	c3                   	ret

080041d1 <wifi_fw_load_chunk>:
 80041d1:	55                   	push   %ebp
 80041d2:	89 e5                	mov    %esp,%ebp
 80041d4:	53                   	push   %ebx
 80041d5:	83 ec 10             	sub    $0x10,%esp
 80041d8:	b8 26 00 00 00       	mov    $0x26,%eax
 80041dd:	8b 5d 08             	mov    0x8(%ebp),%ebx
 80041e0:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 80041e3:	8b 55 10             	mov    0x10(%ebp),%edx
 80041e6:	cd 80                	int    $0x80
 80041e8:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80041eb:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80041ee:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80041f1:	c9                   	leave
 80041f2:	c3                   	ret

080041f3 <wifi_fw_load_end>:
 80041f3:	55                   	push   %ebp
 80041f4:	89 e5                	mov    %esp,%ebp
 80041f6:	83 ec 10             	sub    $0x10,%esp
 80041f9:	b8 27 00 00 00       	mov    $0x27,%eax
 80041fe:	cd 80                	int    $0x80
 8004200:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8004203:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004206:	c9                   	leave
 8004207:	c3                   	ret

08004208 <wifi_fw_load>:
 8004208:	55                   	push   %ebp
 8004209:	89 e5                	mov    %esp,%ebp
 800420b:	83 ec 10             	sub    $0x10,%esp
 800420e:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 8004212:	74 09                	je     800421d <wifi_fw_load+0x15>
 8004214:	81 7d 0c 00 00 20 00 	cmpl   $0x200000,0xc(%ebp)
 800421b:	76 07                	jbe    8004224 <wifi_fw_load+0x1c>
 800421d:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8004222:	eb 73                	jmp    8004297 <wifi_fw_load+0x8f>
 8004224:	ff 75 0c             	push   0xc(%ebp)
 8004227:	e8 87 ff ff ff       	call   80041b3 <wifi_fw_load_begin>
 800422c:	83 c4 04             	add    $0x4,%esp
 800422f:	85 c0                	test   %eax,%eax
 8004231:	79 07                	jns    800423a <wifi_fw_load+0x32>
 8004233:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8004238:	eb 5d                	jmp    8004297 <wifi_fw_load+0x8f>
 800423a:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%ebp)
 8004241:	eb 47                	jmp    800428a <wifi_fw_load+0x82>
 8004243:	8b 45 0c             	mov    0xc(%ebp),%eax
 8004246:	2b 45 fc             	sub    -0x4(%ebp),%eax
 8004249:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800424c:	81 7d f8 00 10 00 00 	cmpl   $0x1000,-0x8(%ebp)
 8004253:	76 07                	jbe    800425c <wifi_fw_load+0x54>
 8004255:	c7 45 f8 00 10 00 00 	movl   $0x1000,-0x8(%ebp)
 800425c:	8b 55 08             	mov    0x8(%ebp),%edx
 800425f:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004262:	01 d0                	add    %edx,%eax
 8004264:	ff 75 fc             	push   -0x4(%ebp)
 8004267:	ff 75 f8             	push   -0x8(%ebp)
 800426a:	50                   	push   %eax
 800426b:	e8 61 ff ff ff       	call   80041d1 <wifi_fw_load_chunk>
 8004270:	83 c4 0c             	add    $0xc,%esp
 8004273:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8004276:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 800427a:	79 07                	jns    8004283 <wifi_fw_load+0x7b>
 800427c:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 8004281:	eb 14                	jmp    8004297 <wifi_fw_load+0x8f>
 8004283:	81 45 fc 00 10 00 00 	addl   $0x1000,-0x4(%ebp)
 800428a:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800428d:	3b 45 0c             	cmp    0xc(%ebp),%eax
 8004290:	72 b1                	jb     8004243 <wifi_fw_load+0x3b>
 8004292:	e8 5c ff ff ff       	call   80041f3 <wifi_fw_load_end>
 8004297:	c9                   	leave
 8004298:	c3                   	ret

08004299 <execv>:
 8004299:	55                   	push   %ebp
 800429a:	89 e5                	mov    %esp,%ebp
 800429c:	53                   	push   %ebx
 800429d:	83 ec 10             	sub    $0x10,%esp
 80042a0:	b8 29 00 00 00       	mov    $0x29,%eax
 80042a5:	8b 55 08             	mov    0x8(%ebp),%edx
 80042a8:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 80042ab:	89 d3                	mov    %edx,%ebx
 80042ad:	cd 80                	int    $0x80
 80042af:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80042b2:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80042b5:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 80042b8:	c9                   	leave
 80042b9:	c3                   	ret

080042ba <msi_test>:
 80042ba:	55                   	push   %ebp
 80042bb:	89 e5                	mov    %esp,%ebp
 80042bd:	83 ec 10             	sub    $0x10,%esp
 80042c0:	b8 3c 00 00 00       	mov    $0x3c,%eax
 80042c5:	cd 80                	int    $0x80
 80042c7:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80042ca:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80042cd:	c9                   	leave
 80042ce:	c3                   	ret

080042cf <e1000_loopback_test>:
 80042cf:	55                   	push   %ebp
 80042d0:	89 e5                	mov    %esp,%ebp
 80042d2:	83 ec 10             	sub    $0x10,%esp
 80042d5:	b8 3d 00 00 00       	mov    $0x3d,%eax
 80042da:	cd 80                	int    $0x80
 80042dc:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80042df:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80042e2:	c9                   	leave
 80042e3:	c3                   	ret

080042e4 <e1000_loopback_test_interrupt>:
 80042e4:	55                   	push   %ebp
 80042e5:	89 e5                	mov    %esp,%ebp
 80042e7:	83 ec 10             	sub    $0x10,%esp
 80042ea:	b8 3e 00 00 00       	mov    $0x3e,%eax
 80042ef:	cd 80                	int    $0x80
 80042f1:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80042f4:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80042f7:	c9                   	leave
 80042f8:	c3                   	ret

080042f9 <gui_get_fb_info>:
 80042f9:	55                   	push   %ebp
 80042fa:	89 e5                	mov    %esp,%ebp
 80042fc:	53                   	push   %ebx
 80042fd:	83 ec 10             	sub    $0x10,%esp
 8004300:	b8 46 00 00 00       	mov    $0x46,%eax
 8004305:	8b 55 08             	mov    0x8(%ebp),%edx
 8004308:	89 d3                	mov    %edx,%ebx
 800430a:	cd 80                	int    $0x80
 800430c:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800430f:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004312:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004315:	c9                   	leave
 8004316:	c3                   	ret

08004317 <gui_fb_blit>:
 8004317:	55                   	push   %ebp
 8004318:	89 e5                	mov    %esp,%ebp
 800431a:	57                   	push   %edi
 800431b:	56                   	push   %esi
 800431c:	53                   	push   %ebx
 800431d:	83 ec 10             	sub    $0x10,%esp
 8004320:	b8 47 00 00 00       	mov    $0x47,%eax
 8004325:	8b 5d 08             	mov    0x8(%ebp),%ebx
 8004328:	8b 4d 0c             	mov    0xc(%ebp),%ecx
 800432b:	8b 55 10             	mov    0x10(%ebp),%edx
 800432e:	8b 75 14             	mov    0x14(%ebp),%esi
 8004331:	8b 7d 18             	mov    0x18(%ebp),%edi
 8004334:	cd 80                	int    $0x80
 8004336:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8004339:	8b 45 f0             	mov    -0x10(%ebp),%eax
 800433c:	83 c4 10             	add    $0x10,%esp
 800433f:	5b                   	pop    %ebx
 8004340:	5e                   	pop    %esi
 8004341:	5f                   	pop    %edi
 8004342:	5d                   	pop    %ebp
 8004343:	c3                   	ret

08004344 <gui_read_input>:
 8004344:	55                   	push   %ebp
 8004345:	89 e5                	mov    %esp,%ebp
 8004347:	53                   	push   %ebx
 8004348:	83 ec 10             	sub    $0x10,%esp
 800434b:	b8 48 00 00 00       	mov    $0x48,%eax
 8004350:	8b 55 08             	mov    0x8(%ebp),%edx
 8004353:	b9 00 00 00 00       	mov    $0x0,%ecx
 8004358:	89 d3                	mov    %edx,%ebx
 800435a:	cd 80                	int    $0x80
 800435c:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800435f:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004362:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004365:	c9                   	leave
 8004366:	c3                   	ret

08004367 <usb_mouse_poll>:
 8004367:	55                   	push   %ebp
 8004368:	89 e5                	mov    %esp,%ebp
 800436a:	53                   	push   %ebx
 800436b:	83 ec 10             	sub    $0x10,%esp
 800436e:	b8 49 00 00 00       	mov    $0x49,%eax
 8004373:	8b 55 08             	mov    0x8(%ebp),%edx
 8004376:	89 d3                	mov    %edx,%ebx
 8004378:	cd 80                	int    $0x80
 800437a:	89 45 f8             	mov    %eax,-0x8(%ebp)
 800437d:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004380:	8b 5d fc             	mov    -0x4(%ebp),%ebx
 8004383:	c9                   	leave
 8004384:	c3                   	ret

08004385 <strchr>:
 8004385:	55                   	push   %ebp
 8004386:	89 e5                	mov    %esp,%ebp
 8004388:	eb 16                	jmp    80043a0 <strchr+0x1b>
 800438a:	8b 45 08             	mov    0x8(%ebp),%eax
 800438d:	0f b6 00             	movzbl (%eax),%eax
 8004390:	8b 55 0c             	mov    0xc(%ebp),%edx
 8004393:	38 d0                	cmp    %dl,%al
 8004395:	75 05                	jne    800439c <strchr+0x17>
 8004397:	8b 45 08             	mov    0x8(%ebp),%eax
 800439a:	eb 13                	jmp    80043af <strchr+0x2a>
 800439c:	83 45 08 01          	addl   $0x1,0x8(%ebp)
 80043a0:	8b 45 08             	mov    0x8(%ebp),%eax
 80043a3:	0f b6 00             	movzbl (%eax),%eax
 80043a6:	84 c0                	test   %al,%al
 80043a8:	75 e0                	jne    800438a <strchr+0x5>
 80043aa:	b8 00 00 00 00       	mov    $0x0,%eax
 80043af:	5d                   	pop    %ebp
 80043b0:	c3                   	ret

080043b1 <strcat>:
 80043b1:	55                   	push   %ebp
 80043b2:	89 e5                	mov    %esp,%ebp
 80043b4:	83 ec 10             	sub    $0x10,%esp
 80043b7:	8b 45 08             	mov    0x8(%ebp),%eax
 80043ba:	89 45 fc             	mov    %eax,-0x4(%ebp)
 80043bd:	eb 04                	jmp    80043c3 <strcat+0x12>
 80043bf:	83 45 fc 01          	addl   $0x1,-0x4(%ebp)
 80043c3:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80043c6:	0f b6 00             	movzbl (%eax),%eax
 80043c9:	84 c0                	test   %al,%al
 80043cb:	75 f2                	jne    80043bf <strcat+0xe>
 80043cd:	90                   	nop
 80043ce:	8b 55 0c             	mov    0xc(%ebp),%edx
 80043d1:	8d 42 01             	lea    0x1(%edx),%eax
 80043d4:	89 45 0c             	mov    %eax,0xc(%ebp)
 80043d7:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80043da:	8d 48 01             	lea    0x1(%eax),%ecx
 80043dd:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 80043e0:	0f b6 12             	movzbl (%edx),%edx
 80043e3:	88 10                	mov    %dl,(%eax)
 80043e5:	0f b6 00             	movzbl (%eax),%eax
 80043e8:	84 c0                	test   %al,%al
 80043ea:	75 e2                	jne    80043ce <strcat+0x1d>
 80043ec:	8b 45 08             	mov    0x8(%ebp),%eax
 80043ef:	c9                   	leave
 80043f0:	c3                   	ret

080043f1 <sprintf>:
 80043f1:	55                   	push   %ebp
 80043f2:	89 e5                	mov    %esp,%ebp
 80043f4:	83 ec 60             	sub    $0x60,%esp
 80043f7:	8d 45 10             	lea    0x10(%ebp),%eax
 80043fa:	89 45 d0             	mov    %eax,-0x30(%ebp)
 80043fd:	8b 45 08             	mov    0x8(%ebp),%eax
 8004400:	89 45 fc             	mov    %eax,-0x4(%ebp)
 8004403:	8b 45 0c             	mov    0xc(%ebp),%eax
 8004406:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8004409:	e9 e5 02 00 00       	jmp    80046f3 <sprintf+0x302>
 800440e:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004411:	0f b6 00             	movzbl (%eax),%eax
 8004414:	3c 25                	cmp    $0x25,%al
 8004416:	0f 85 c0 02 00 00    	jne    80046dc <sprintf+0x2eb>
 800441c:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
 8004420:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004423:	0f b6 00             	movzbl (%eax),%eax
 8004426:	0f be c0             	movsbl %al,%eax
 8004429:	83 f8 78             	cmp    $0x78,%eax
 800442c:	0f 8f 86 02 00 00    	jg     80046b8 <sprintf+0x2c7>
 8004432:	83 f8 63             	cmp    $0x63,%eax
 8004435:	7d 16                	jge    800444d <sprintf+0x5c>
 8004437:	85 c0                	test   %eax,%eax
 8004439:	0f 84 6b 02 00 00    	je     80046aa <sprintf+0x2b9>
 800443f:	83 f8 25             	cmp    $0x25,%eax
 8004442:	0f 84 54 02 00 00    	je     800469c <sprintf+0x2ab>
 8004448:	e9 6b 02 00 00       	jmp    80046b8 <sprintf+0x2c7>
 800444d:	83 e8 63             	sub    $0x63,%eax
 8004450:	83 f8 15             	cmp    $0x15,%eax
 8004453:	0f 87 5f 02 00 00    	ja     80046b8 <sprintf+0x2c7>
 8004459:	8b 04 85 30 57 00 08 	mov    0x8005730(,%eax,4),%eax
 8004460:	ff e0                	jmp    *%eax
 8004462:	8b 45 d0             	mov    -0x30(%ebp),%eax
 8004465:	8d 50 04             	lea    0x4(%eax),%edx
 8004468:	89 55 d0             	mov    %edx,-0x30(%ebp)
 800446b:	8b 00                	mov    (%eax),%eax
 800446d:	89 45 f4             	mov    %eax,-0xc(%ebp)
 8004470:	eb 17                	jmp    8004489 <sprintf+0x98>
 8004472:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8004475:	8d 42 01             	lea    0x1(%edx),%eax
 8004478:	89 45 f4             	mov    %eax,-0xc(%ebp)
 800447b:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800447e:	8d 48 01             	lea    0x1(%eax),%ecx
 8004481:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 8004484:	0f b6 12             	movzbl (%edx),%edx
 8004487:	88 10                	mov    %dl,(%eax)
 8004489:	8b 45 f4             	mov    -0xc(%ebp),%eax
 800448c:	0f b6 00             	movzbl (%eax),%eax
 800448f:	84 c0                	test   %al,%al
 8004491:	75 df                	jne    8004472 <sprintf+0x81>
 8004493:	e9 3e 02 00 00       	jmp    80046d6 <sprintf+0x2e5>
 8004498:	8b 45 d0             	mov    -0x30(%ebp),%eax
 800449b:	8d 50 04             	lea    0x4(%eax),%edx
 800449e:	89 55 d0             	mov    %edx,-0x30(%ebp)
 80044a1:	8b 00                	mov    (%eax),%eax
 80044a3:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80044a6:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 80044aa:	79 0f                	jns    80044bb <sprintf+0xca>
 80044ac:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80044af:	8d 50 01             	lea    0x1(%eax),%edx
 80044b2:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80044b5:	c6 00 2d             	movb   $0x2d,(%eax)
 80044b8:	f7 5d f0             	negl   -0x10(%ebp)
 80044bb:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
 80044c2:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 80044c6:	75 62                	jne    800452a <sprintf+0x139>
 80044c8:	8b 45 ec             	mov    -0x14(%ebp),%eax
 80044cb:	8d 50 01             	lea    0x1(%eax),%edx
 80044ce:	89 55 ec             	mov    %edx,-0x14(%ebp)
 80044d1:	c6 44 05 c0 30       	movb   $0x30,-0x40(%ebp,%eax,1)
 80044d6:	eb 74                	jmp    800454c <sprintf+0x15b>
 80044d8:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 80044db:	ba 67 66 66 66       	mov    $0x66666667,%edx
 80044e0:	89 c8                	mov    %ecx,%eax
 80044e2:	f7 ea                	imul   %edx
 80044e4:	c1 fa 02             	sar    $0x2,%edx
 80044e7:	89 c8                	mov    %ecx,%eax
 80044e9:	c1 f8 1f             	sar    $0x1f,%eax
 80044ec:	29 c2                	sub    %eax,%edx
 80044ee:	89 d0                	mov    %edx,%eax
 80044f0:	c1 e0 02             	shl    $0x2,%eax
 80044f3:	01 d0                	add    %edx,%eax
 80044f5:	01 c0                	add    %eax,%eax
 80044f7:	29 c1                	sub    %eax,%ecx
 80044f9:	89 ca                	mov    %ecx,%edx
 80044fb:	89 d0                	mov    %edx,%eax
 80044fd:	8d 48 30             	lea    0x30(%eax),%ecx
 8004500:	8b 45 ec             	mov    -0x14(%ebp),%eax
 8004503:	8d 50 01             	lea    0x1(%eax),%edx
 8004506:	89 55 ec             	mov    %edx,-0x14(%ebp)
 8004509:	89 ca                	mov    %ecx,%edx
 800450b:	88 54 05 c0          	mov    %dl,-0x40(%ebp,%eax,1)
 800450f:	8b 4d f0             	mov    -0x10(%ebp),%ecx
 8004512:	ba 67 66 66 66       	mov    $0x66666667,%edx
 8004517:	89 c8                	mov    %ecx,%eax
 8004519:	f7 ea                	imul   %edx
 800451b:	89 d0                	mov    %edx,%eax
 800451d:	c1 f8 02             	sar    $0x2,%eax
 8004520:	c1 f9 1f             	sar    $0x1f,%ecx
 8004523:	89 ca                	mov    %ecx,%edx
 8004525:	29 d0                	sub    %edx,%eax
 8004527:	89 45 f0             	mov    %eax,-0x10(%ebp)
 800452a:	83 7d f0 00          	cmpl   $0x0,-0x10(%ebp)
 800452e:	7f a8                	jg     80044d8 <sprintf+0xe7>
 8004530:	eb 1a                	jmp    800454c <sprintf+0x15b>
 8004532:	83 6d ec 01          	subl   $0x1,-0x14(%ebp)
 8004536:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004539:	8d 50 01             	lea    0x1(%eax),%edx
 800453c:	89 55 fc             	mov    %edx,-0x4(%ebp)
 800453f:	8d 4d c0             	lea    -0x40(%ebp),%ecx
 8004542:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8004545:	01 ca                	add    %ecx,%edx
 8004547:	0f b6 12             	movzbl (%edx),%edx
 800454a:	88 10                	mov    %dl,(%eax)
 800454c:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
 8004550:	7f e0                	jg     8004532 <sprintf+0x141>
 8004552:	e9 7f 01 00 00       	jmp    80046d6 <sprintf+0x2e5>
 8004557:	8b 45 d0             	mov    -0x30(%ebp),%eax
 800455a:	8d 50 04             	lea    0x4(%eax),%edx
 800455d:	89 55 d0             	mov    %edx,-0x30(%ebp)
 8004560:	8b 00                	mov    (%eax),%eax
 8004562:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8004565:	c7 45 e4 00 00 00 00 	movl   $0x0,-0x1c(%ebp)
 800456c:	83 7d e8 00          	cmpl   $0x0,-0x18(%ebp)
 8004570:	75 52                	jne    80045c4 <sprintf+0x1d3>
 8004572:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 8004575:	8d 50 01             	lea    0x1(%eax),%edx
 8004578:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 800457b:	c6 44 05 b0 30       	movb   $0x30,-0x50(%ebp,%eax,1)
 8004580:	eb 64                	jmp    80045e6 <sprintf+0x1f5>
 8004582:	8b 4d e8             	mov    -0x18(%ebp),%ecx
 8004585:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
 800458a:	89 c8                	mov    %ecx,%eax
 800458c:	f7 e2                	mul    %edx
 800458e:	c1 ea 03             	shr    $0x3,%edx
 8004591:	89 d0                	mov    %edx,%eax
 8004593:	c1 e0 02             	shl    $0x2,%eax
 8004596:	01 d0                	add    %edx,%eax
 8004598:	01 c0                	add    %eax,%eax
 800459a:	29 c1                	sub    %eax,%ecx
 800459c:	89 ca                	mov    %ecx,%edx
 800459e:	89 d0                	mov    %edx,%eax
 80045a0:	8d 48 30             	lea    0x30(%eax),%ecx
 80045a3:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 80045a6:	8d 50 01             	lea    0x1(%eax),%edx
 80045a9:	89 55 e4             	mov    %edx,-0x1c(%ebp)
 80045ac:	89 ca                	mov    %ecx,%edx
 80045ae:	88 54 05 b0          	mov    %dl,-0x50(%ebp,%eax,1)
 80045b2:	8b 45 e8             	mov    -0x18(%ebp),%eax
 80045b5:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
 80045ba:	f7 e2                	mul    %edx
 80045bc:	89 d0                	mov    %edx,%eax
 80045be:	c1 e8 03             	shr    $0x3,%eax
 80045c1:	89 45 e8             	mov    %eax,-0x18(%ebp)
 80045c4:	83 7d e8 00          	cmpl   $0x0,-0x18(%ebp)
 80045c8:	75 b8                	jne    8004582 <sprintf+0x191>
 80045ca:	eb 1a                	jmp    80045e6 <sprintf+0x1f5>
 80045cc:	83 6d e4 01          	subl   $0x1,-0x1c(%ebp)
 80045d0:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80045d3:	8d 50 01             	lea    0x1(%eax),%edx
 80045d6:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80045d9:	8d 4d b0             	lea    -0x50(%ebp),%ecx
 80045dc:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 80045df:	01 ca                	add    %ecx,%edx
 80045e1:	0f b6 12             	movzbl (%edx),%edx
 80045e4:	88 10                	mov    %dl,(%eax)
 80045e6:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
 80045ea:	7f e0                	jg     80045cc <sprintf+0x1db>
 80045ec:	e9 e5 00 00 00       	jmp    80046d6 <sprintf+0x2e5>
 80045f1:	8b 45 d0             	mov    -0x30(%ebp),%eax
 80045f4:	8d 50 04             	lea    0x4(%eax),%edx
 80045f7:	89 55 d0             	mov    %edx,-0x30(%ebp)
 80045fa:	8b 00                	mov    (%eax),%eax
 80045fc:	89 45 e0             	mov    %eax,-0x20(%ebp)
 80045ff:	c7 45 dc 00 00 00 00 	movl   $0x0,-0x24(%ebp)
 8004606:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
 800460a:	75 47                	jne    8004653 <sprintf+0x262>
 800460c:	8b 45 dc             	mov    -0x24(%ebp),%eax
 800460f:	8d 50 01             	lea    0x1(%eax),%edx
 8004612:	89 55 dc             	mov    %edx,-0x24(%ebp)
 8004615:	c6 44 05 a0 30       	movb   $0x30,-0x60(%ebp,%eax,1)
 800461a:	eb 59                	jmp    8004675 <sprintf+0x284>
 800461c:	8b 45 e0             	mov    -0x20(%ebp),%eax
 800461f:	83 e0 0f             	and    $0xf,%eax
 8004622:	89 45 d8             	mov    %eax,-0x28(%ebp)
 8004625:	83 7d d8 09          	cmpl   $0x9,-0x28(%ebp)
 8004629:	7f 0a                	jg     8004635 <sprintf+0x244>
 800462b:	8b 45 d8             	mov    -0x28(%ebp),%eax
 800462e:	83 c0 30             	add    $0x30,%eax
 8004631:	89 c1                	mov    %eax,%ecx
 8004633:	eb 08                	jmp    800463d <sprintf+0x24c>
 8004635:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8004638:	83 c0 57             	add    $0x57,%eax
 800463b:	89 c1                	mov    %eax,%ecx
 800463d:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8004640:	8d 50 01             	lea    0x1(%eax),%edx
 8004643:	89 55 dc             	mov    %edx,-0x24(%ebp)
 8004646:	88 4c 05 a0          	mov    %cl,-0x60(%ebp,%eax,1)
 800464a:	8b 45 e0             	mov    -0x20(%ebp),%eax
 800464d:	c1 e8 04             	shr    $0x4,%eax
 8004650:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8004653:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
 8004657:	75 c3                	jne    800461c <sprintf+0x22b>
 8004659:	eb 1a                	jmp    8004675 <sprintf+0x284>
 800465b:	83 6d dc 01          	subl   $0x1,-0x24(%ebp)
 800465f:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004662:	8d 50 01             	lea    0x1(%eax),%edx
 8004665:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004668:	8d 4d a0             	lea    -0x60(%ebp),%ecx
 800466b:	8b 55 dc             	mov    -0x24(%ebp),%edx
 800466e:	01 ca                	add    %ecx,%edx
 8004670:	0f b6 12             	movzbl (%edx),%edx
 8004673:	88 10                	mov    %dl,(%eax)
 8004675:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 8004679:	7f e0                	jg     800465b <sprintf+0x26a>
 800467b:	eb 59                	jmp    80046d6 <sprintf+0x2e5>
 800467d:	8b 45 d0             	mov    -0x30(%ebp),%eax
 8004680:	8d 50 04             	lea    0x4(%eax),%edx
 8004683:	89 55 d0             	mov    %edx,-0x30(%ebp)
 8004686:	8b 00                	mov    (%eax),%eax
 8004688:	88 45 d7             	mov    %al,-0x29(%ebp)
 800468b:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800468e:	8d 50 01             	lea    0x1(%eax),%edx
 8004691:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004694:	0f b6 55 d7          	movzbl -0x29(%ebp),%edx
 8004698:	88 10                	mov    %dl,(%eax)
 800469a:	eb 3a                	jmp    80046d6 <sprintf+0x2e5>
 800469c:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800469f:	8d 50 01             	lea    0x1(%eax),%edx
 80046a2:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80046a5:	c6 00 25             	movb   $0x25,(%eax)
 80046a8:	eb 2c                	jmp    80046d6 <sprintf+0x2e5>
 80046aa:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80046ad:	8d 50 01             	lea    0x1(%eax),%edx
 80046b0:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80046b3:	c6 00 25             	movb   $0x25,(%eax)
 80046b6:	eb 4a                	jmp    8004702 <sprintf+0x311>
 80046b8:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80046bb:	8d 50 01             	lea    0x1(%eax),%edx
 80046be:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80046c1:	c6 00 25             	movb   $0x25,(%eax)
 80046c4:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80046c7:	8d 50 01             	lea    0x1(%eax),%edx
 80046ca:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80046cd:	8b 55 f8             	mov    -0x8(%ebp),%edx
 80046d0:	0f b6 12             	movzbl (%edx),%edx
 80046d3:	88 10                	mov    %dl,(%eax)
 80046d5:	90                   	nop
 80046d6:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
 80046da:	eb 17                	jmp    80046f3 <sprintf+0x302>
 80046dc:	8b 55 f8             	mov    -0x8(%ebp),%edx
 80046df:	8d 42 01             	lea    0x1(%edx),%eax
 80046e2:	89 45 f8             	mov    %eax,-0x8(%ebp)
 80046e5:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80046e8:	8d 48 01             	lea    0x1(%eax),%ecx
 80046eb:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 80046ee:	0f b6 12             	movzbl (%edx),%edx
 80046f1:	88 10                	mov    %dl,(%eax)
 80046f3:	8b 45 f8             	mov    -0x8(%ebp),%eax
 80046f6:	0f b6 00             	movzbl (%eax),%eax
 80046f9:	84 c0                	test   %al,%al
 80046fb:	0f 85 0d fd ff ff    	jne    800440e <sprintf+0x1d>
 8004701:	90                   	nop
 8004702:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004705:	c6 00 00             	movb   $0x0,(%eax)
 8004708:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800470b:	2b 45 08             	sub    0x8(%ebp),%eax
 800470e:	c9                   	leave
 800470f:	c3                   	ret

08004710 <vsnprintf>:
 8004710:	55                   	push   %ebp
 8004711:	89 e5                	mov    %esp,%ebp
 8004713:	83 ec 60             	sub    $0x60,%esp
 8004716:	83 7d 0c 00          	cmpl   $0x0,0xc(%ebp)
 800471a:	75 0a                	jne    8004726 <vsnprintf+0x16>
 800471c:	b8 00 00 00 00       	mov    $0x0,%eax
 8004721:	e9 d7 03 00 00       	jmp    8004afd <vsnprintf+0x3ed>
 8004726:	8b 45 08             	mov    0x8(%ebp),%eax
 8004729:	89 45 fc             	mov    %eax,-0x4(%ebp)
 800472c:	8b 45 10             	mov    0x10(%ebp),%eax
 800472f:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8004732:	8b 45 0c             	mov    0xc(%ebp),%eax
 8004735:	83 e8 01             	sub    $0x1,%eax
 8004738:	89 45 f4             	mov    %eax,-0xc(%ebp)
 800473b:	e9 94 03 00 00       	jmp    8004ad4 <vsnprintf+0x3c4>
 8004740:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004743:	0f b6 00             	movzbl (%eax),%eax
 8004746:	3c 25                	cmp    $0x25,%al
 8004748:	0f 85 6b 03 00 00    	jne    8004ab9 <vsnprintf+0x3a9>
 800474e:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
 8004752:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004755:	0f b6 00             	movzbl (%eax),%eax
 8004758:	0f be c0             	movsbl %al,%eax
 800475b:	83 f8 78             	cmp    $0x78,%eax
 800475e:	0f 8f fb 02 00 00    	jg     8004a5f <vsnprintf+0x34f>
 8004764:	83 f8 63             	cmp    $0x63,%eax
 8004767:	7d 16                	jge    800477f <vsnprintf+0x6f>
 8004769:	85 c0                	test   %eax,%eax
 800476b:	0f 84 cf 02 00 00    	je     8004a40 <vsnprintf+0x330>
 8004771:	83 f8 25             	cmp    $0x25,%eax
 8004774:	0f 84 aa 02 00 00    	je     8004a24 <vsnprintf+0x314>
 800477a:	e9 e0 02 00 00       	jmp    8004a5f <vsnprintf+0x34f>
 800477f:	83 e8 63             	sub    $0x63,%eax
 8004782:	83 f8 15             	cmp    $0x15,%eax
 8004785:	0f 87 d4 02 00 00    	ja     8004a5f <vsnprintf+0x34f>
 800478b:	8b 04 85 88 57 00 08 	mov    0x8005788(,%eax,4),%eax
 8004792:	ff e0                	jmp    *%eax
 8004794:	8b 45 14             	mov    0x14(%ebp),%eax
 8004797:	8d 50 04             	lea    0x4(%eax),%edx
 800479a:	89 55 14             	mov    %edx,0x14(%ebp)
 800479d:	8b 00                	mov    (%eax),%eax
 800479f:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80047a2:	eb 1b                	jmp    80047bf <vsnprintf+0xaf>
 80047a4:	8b 55 f0             	mov    -0x10(%ebp),%edx
 80047a7:	8d 42 01             	lea    0x1(%edx),%eax
 80047aa:	89 45 f0             	mov    %eax,-0x10(%ebp)
 80047ad:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80047b0:	8d 48 01             	lea    0x1(%eax),%ecx
 80047b3:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 80047b6:	0f b6 12             	movzbl (%edx),%edx
 80047b9:	88 10                	mov    %dl,(%eax)
 80047bb:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 80047bf:	8b 45 f0             	mov    -0x10(%ebp),%eax
 80047c2:	0f b6 00             	movzbl (%eax),%eax
 80047c5:	84 c0                	test   %al,%al
 80047c7:	0f 84 d3 02 00 00    	je     8004aa0 <vsnprintf+0x390>
 80047cd:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80047d1:	75 d1                	jne    80047a4 <vsnprintf+0x94>
 80047d3:	e9 c8 02 00 00       	jmp    8004aa0 <vsnprintf+0x390>
 80047d8:	8b 45 14             	mov    0x14(%ebp),%eax
 80047db:	8d 50 04             	lea    0x4(%eax),%edx
 80047de:	89 55 14             	mov    %edx,0x14(%ebp)
 80047e1:	8b 00                	mov    (%eax),%eax
 80047e3:	89 45 ec             	mov    %eax,-0x14(%ebp)
 80047e6:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
 80047ea:	79 19                	jns    8004805 <vsnprintf+0xf5>
 80047ec:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80047f0:	74 13                	je     8004805 <vsnprintf+0xf5>
 80047f2:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80047f5:	8d 50 01             	lea    0x1(%eax),%edx
 80047f8:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80047fb:	c6 00 2d             	movb   $0x2d,(%eax)
 80047fe:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004802:	f7 5d ec             	negl   -0x14(%ebp)
 8004805:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
 800480c:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
 8004810:	75 62                	jne    8004874 <vsnprintf+0x164>
 8004812:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8004815:	8d 50 01             	lea    0x1(%eax),%edx
 8004818:	89 55 e8             	mov    %edx,-0x18(%ebp)
 800481b:	c6 44 05 c3 30       	movb   $0x30,-0x3d(%ebp,%eax,1)
 8004820:	eb 78                	jmp    800489a <vsnprintf+0x18a>
 8004822:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 8004825:	ba 67 66 66 66       	mov    $0x66666667,%edx
 800482a:	89 c8                	mov    %ecx,%eax
 800482c:	f7 ea                	imul   %edx
 800482e:	c1 fa 02             	sar    $0x2,%edx
 8004831:	89 c8                	mov    %ecx,%eax
 8004833:	c1 f8 1f             	sar    $0x1f,%eax
 8004836:	29 c2                	sub    %eax,%edx
 8004838:	89 d0                	mov    %edx,%eax
 800483a:	c1 e0 02             	shl    $0x2,%eax
 800483d:	01 d0                	add    %edx,%eax
 800483f:	01 c0                	add    %eax,%eax
 8004841:	29 c1                	sub    %eax,%ecx
 8004843:	89 ca                	mov    %ecx,%edx
 8004845:	89 d0                	mov    %edx,%eax
 8004847:	8d 48 30             	lea    0x30(%eax),%ecx
 800484a:	8b 45 e8             	mov    -0x18(%ebp),%eax
 800484d:	8d 50 01             	lea    0x1(%eax),%edx
 8004850:	89 55 e8             	mov    %edx,-0x18(%ebp)
 8004853:	89 ca                	mov    %ecx,%edx
 8004855:	88 54 05 c3          	mov    %dl,-0x3d(%ebp,%eax,1)
 8004859:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 800485c:	ba 67 66 66 66       	mov    $0x66666667,%edx
 8004861:	89 c8                	mov    %ecx,%eax
 8004863:	f7 ea                	imul   %edx
 8004865:	89 d0                	mov    %edx,%eax
 8004867:	c1 f8 02             	sar    $0x2,%eax
 800486a:	c1 f9 1f             	sar    $0x1f,%ecx
 800486d:	89 ca                	mov    %ecx,%edx
 800486f:	29 d0                	sub    %edx,%eax
 8004871:	89 45 ec             	mov    %eax,-0x14(%ebp)
 8004874:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
 8004878:	7f a8                	jg     8004822 <vsnprintf+0x112>
 800487a:	eb 1e                	jmp    800489a <vsnprintf+0x18a>
 800487c:	83 6d e8 01          	subl   $0x1,-0x18(%ebp)
 8004880:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004883:	8d 50 01             	lea    0x1(%eax),%edx
 8004886:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004889:	8d 4d c3             	lea    -0x3d(%ebp),%ecx
 800488c:	8b 55 e8             	mov    -0x18(%ebp),%edx
 800488f:	01 ca                	add    %ecx,%edx
 8004891:	0f b6 12             	movzbl (%edx),%edx
 8004894:	88 10                	mov    %dl,(%eax)
 8004896:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 800489a:	83 7d e8 00          	cmpl   $0x0,-0x18(%ebp)
 800489e:	0f 8e ff 01 00 00    	jle    8004aa3 <vsnprintf+0x393>
 80048a4:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80048a8:	75 d2                	jne    800487c <vsnprintf+0x16c>
 80048aa:	e9 f4 01 00 00       	jmp    8004aa3 <vsnprintf+0x393>
 80048af:	8b 45 14             	mov    0x14(%ebp),%eax
 80048b2:	8d 50 04             	lea    0x4(%eax),%edx
 80048b5:	89 55 14             	mov    %edx,0x14(%ebp)
 80048b8:	8b 00                	mov    (%eax),%eax
 80048ba:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 80048bd:	c7 45 e0 00 00 00 00 	movl   $0x0,-0x20(%ebp)
 80048c4:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
 80048c8:	75 52                	jne    800491c <vsnprintf+0x20c>
 80048ca:	8b 45 e0             	mov    -0x20(%ebp),%eax
 80048cd:	8d 50 01             	lea    0x1(%eax),%edx
 80048d0:	89 55 e0             	mov    %edx,-0x20(%ebp)
 80048d3:	c6 44 05 b3 30       	movb   $0x30,-0x4d(%ebp,%eax,1)
 80048d8:	eb 68                	jmp    8004942 <vsnprintf+0x232>
 80048da:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
 80048dd:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
 80048e2:	89 c8                	mov    %ecx,%eax
 80048e4:	f7 e2                	mul    %edx
 80048e6:	c1 ea 03             	shr    $0x3,%edx
 80048e9:	89 d0                	mov    %edx,%eax
 80048eb:	c1 e0 02             	shl    $0x2,%eax
 80048ee:	01 d0                	add    %edx,%eax
 80048f0:	01 c0                	add    %eax,%eax
 80048f2:	29 c1                	sub    %eax,%ecx
 80048f4:	89 ca                	mov    %ecx,%edx
 80048f6:	89 d0                	mov    %edx,%eax
 80048f8:	8d 48 30             	lea    0x30(%eax),%ecx
 80048fb:	8b 45 e0             	mov    -0x20(%ebp),%eax
 80048fe:	8d 50 01             	lea    0x1(%eax),%edx
 8004901:	89 55 e0             	mov    %edx,-0x20(%ebp)
 8004904:	89 ca                	mov    %ecx,%edx
 8004906:	88 54 05 b3          	mov    %dl,-0x4d(%ebp,%eax,1)
 800490a:	8b 45 e4             	mov    -0x1c(%ebp),%eax
 800490d:	ba cd cc cc cc       	mov    $0xcccccccd,%edx
 8004912:	f7 e2                	mul    %edx
 8004914:	89 d0                	mov    %edx,%eax
 8004916:	c1 e8 03             	shr    $0x3,%eax
 8004919:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 800491c:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
 8004920:	75 b8                	jne    80048da <vsnprintf+0x1ca>
 8004922:	eb 1e                	jmp    8004942 <vsnprintf+0x232>
 8004924:	83 6d e0 01          	subl   $0x1,-0x20(%ebp)
 8004928:	8b 45 fc             	mov    -0x4(%ebp),%eax
 800492b:	8d 50 01             	lea    0x1(%eax),%edx
 800492e:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004931:	8d 4d b3             	lea    -0x4d(%ebp),%ecx
 8004934:	8b 55 e0             	mov    -0x20(%ebp),%edx
 8004937:	01 ca                	add    %ecx,%edx
 8004939:	0f b6 12             	movzbl (%edx),%edx
 800493c:	88 10                	mov    %dl,(%eax)
 800493e:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004942:	83 7d e0 00          	cmpl   $0x0,-0x20(%ebp)
 8004946:	0f 8e 5a 01 00 00    	jle    8004aa6 <vsnprintf+0x396>
 800494c:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 8004950:	75 d2                	jne    8004924 <vsnprintf+0x214>
 8004952:	e9 4f 01 00 00       	jmp    8004aa6 <vsnprintf+0x396>
 8004957:	8b 45 14             	mov    0x14(%ebp),%eax
 800495a:	8d 50 04             	lea    0x4(%eax),%edx
 800495d:	89 55 14             	mov    %edx,0x14(%ebp)
 8004960:	8b 00                	mov    (%eax),%eax
 8004962:	89 45 dc             	mov    %eax,-0x24(%ebp)
 8004965:	c7 45 d8 00 00 00 00 	movl   $0x0,-0x28(%ebp)
 800496c:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 8004970:	75 47                	jne    80049b9 <vsnprintf+0x2a9>
 8004972:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8004975:	8d 50 01             	lea    0x1(%eax),%edx
 8004978:	89 55 d8             	mov    %edx,-0x28(%ebp)
 800497b:	c6 44 05 a3 30       	movb   $0x30,-0x5d(%ebp,%eax,1)
 8004980:	eb 5d                	jmp    80049df <vsnprintf+0x2cf>
 8004982:	8b 45 dc             	mov    -0x24(%ebp),%eax
 8004985:	83 e0 0f             	and    $0xf,%eax
 8004988:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 800498b:	83 7d d4 09          	cmpl   $0x9,-0x2c(%ebp)
 800498f:	7f 0a                	jg     800499b <vsnprintf+0x28b>
 8004991:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 8004994:	83 c0 30             	add    $0x30,%eax
 8004997:	89 c1                	mov    %eax,%ecx
 8004999:	eb 08                	jmp    80049a3 <vsnprintf+0x293>
 800499b:	8b 45 d4             	mov    -0x2c(%ebp),%eax
 800499e:	83 c0 57             	add    $0x57,%eax
 80049a1:	89 c1                	mov    %eax,%ecx
 80049a3:	8b 45 d8             	mov    -0x28(%ebp),%eax
 80049a6:	8d 50 01             	lea    0x1(%eax),%edx
 80049a9:	89 55 d8             	mov    %edx,-0x28(%ebp)
 80049ac:	88 4c 05 a3          	mov    %cl,-0x5d(%ebp,%eax,1)
 80049b0:	8b 45 dc             	mov    -0x24(%ebp),%eax
 80049b3:	c1 e8 04             	shr    $0x4,%eax
 80049b6:	89 45 dc             	mov    %eax,-0x24(%ebp)
 80049b9:	83 7d dc 00          	cmpl   $0x0,-0x24(%ebp)
 80049bd:	75 c3                	jne    8004982 <vsnprintf+0x272>
 80049bf:	eb 1e                	jmp    80049df <vsnprintf+0x2cf>
 80049c1:	83 6d d8 01          	subl   $0x1,-0x28(%ebp)
 80049c5:	8b 45 fc             	mov    -0x4(%ebp),%eax
 80049c8:	8d 50 01             	lea    0x1(%eax),%edx
 80049cb:	89 55 fc             	mov    %edx,-0x4(%ebp)
 80049ce:	8d 4d a3             	lea    -0x5d(%ebp),%ecx
 80049d1:	8b 55 d8             	mov    -0x28(%ebp),%edx
 80049d4:	01 ca                	add    %ecx,%edx
 80049d6:	0f b6 12             	movzbl (%edx),%edx
 80049d9:	88 10                	mov    %dl,(%eax)
 80049db:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 80049df:	83 7d d8 00          	cmpl   $0x0,-0x28(%ebp)
 80049e3:	0f 8e c0 00 00 00    	jle    8004aa9 <vsnprintf+0x399>
 80049e9:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80049ed:	75 d2                	jne    80049c1 <vsnprintf+0x2b1>
 80049ef:	e9 b5 00 00 00       	jmp    8004aa9 <vsnprintf+0x399>
 80049f4:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 80049f8:	0f 84 ae 00 00 00    	je     8004aac <vsnprintf+0x39c>
 80049fe:	8b 45 14             	mov    0x14(%ebp),%eax
 8004a01:	8d 50 04             	lea    0x4(%eax),%edx
 8004a04:	89 55 14             	mov    %edx,0x14(%ebp)
 8004a07:	8b 00                	mov    (%eax),%eax
 8004a09:	88 45 d3             	mov    %al,-0x2d(%ebp)
 8004a0c:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004a0f:	8d 50 01             	lea    0x1(%eax),%edx
 8004a12:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004a15:	0f b6 55 d3          	movzbl -0x2d(%ebp),%edx
 8004a19:	88 10                	mov    %dl,(%eax)
 8004a1b:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004a1f:	e9 88 00 00 00       	jmp    8004aac <vsnprintf+0x39c>
 8004a24:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 8004a28:	0f 84 81 00 00 00    	je     8004aaf <vsnprintf+0x39f>
 8004a2e:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004a31:	8d 50 01             	lea    0x1(%eax),%edx
 8004a34:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004a37:	c6 00 25             	movb   $0x25,(%eax)
 8004a3a:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004a3e:	eb 6f                	jmp    8004aaf <vsnprintf+0x39f>
 8004a40:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 8004a44:	0f 84 a0 00 00 00    	je     8004aea <vsnprintf+0x3da>
 8004a4a:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004a4d:	8d 50 01             	lea    0x1(%eax),%edx
 8004a50:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004a53:	c6 00 25             	movb   $0x25,(%eax)
 8004a56:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004a5a:	e9 8b 00 00 00       	jmp    8004aea <vsnprintf+0x3da>
 8004a5f:	83 7d f4 01          	cmpl   $0x1,-0xc(%ebp)
 8004a63:	76 23                	jbe    8004a88 <vsnprintf+0x378>
 8004a65:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004a68:	8d 50 01             	lea    0x1(%eax),%edx
 8004a6b:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004a6e:	c6 00 25             	movb   $0x25,(%eax)
 8004a71:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004a74:	8d 50 01             	lea    0x1(%eax),%edx
 8004a77:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004a7a:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8004a7d:	0f b6 12             	movzbl (%edx),%edx
 8004a80:	88 10                	mov    %dl,(%eax)
 8004a82:	83 6d f4 02          	subl   $0x2,-0xc(%ebp)
 8004a86:	eb 2a                	jmp    8004ab2 <vsnprintf+0x3a2>
 8004a88:	83 7d f4 01          	cmpl   $0x1,-0xc(%ebp)
 8004a8c:	75 24                	jne    8004ab2 <vsnprintf+0x3a2>
 8004a8e:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004a91:	8d 50 01             	lea    0x1(%eax),%edx
 8004a94:	89 55 fc             	mov    %edx,-0x4(%ebp)
 8004a97:	c6 00 25             	movb   $0x25,(%eax)
 8004a9a:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004a9e:	eb 12                	jmp    8004ab2 <vsnprintf+0x3a2>
 8004aa0:	90                   	nop
 8004aa1:	eb 10                	jmp    8004ab3 <vsnprintf+0x3a3>
 8004aa3:	90                   	nop
 8004aa4:	eb 0d                	jmp    8004ab3 <vsnprintf+0x3a3>
 8004aa6:	90                   	nop
 8004aa7:	eb 0a                	jmp    8004ab3 <vsnprintf+0x3a3>
 8004aa9:	90                   	nop
 8004aaa:	eb 07                	jmp    8004ab3 <vsnprintf+0x3a3>
 8004aac:	90                   	nop
 8004aad:	eb 04                	jmp    8004ab3 <vsnprintf+0x3a3>
 8004aaf:	90                   	nop
 8004ab0:	eb 01                	jmp    8004ab3 <vsnprintf+0x3a3>
 8004ab2:	90                   	nop
 8004ab3:	83 45 f8 01          	addl   $0x1,-0x8(%ebp)
 8004ab7:	eb 1b                	jmp    8004ad4 <vsnprintf+0x3c4>
 8004ab9:	8b 55 f8             	mov    -0x8(%ebp),%edx
 8004abc:	8d 42 01             	lea    0x1(%edx),%eax
 8004abf:	89 45 f8             	mov    %eax,-0x8(%ebp)
 8004ac2:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004ac5:	8d 48 01             	lea    0x1(%eax),%ecx
 8004ac8:	89 4d fc             	mov    %ecx,-0x4(%ebp)
 8004acb:	0f b6 12             	movzbl (%edx),%edx
 8004ace:	88 10                	mov    %dl,(%eax)
 8004ad0:	83 6d f4 01          	subl   $0x1,-0xc(%ebp)
 8004ad4:	8b 45 f8             	mov    -0x8(%ebp),%eax
 8004ad7:	0f b6 00             	movzbl (%eax),%eax
 8004ada:	84 c0                	test   %al,%al
 8004adc:	74 0f                	je     8004aed <vsnprintf+0x3dd>
 8004ade:	83 7d f4 00          	cmpl   $0x0,-0xc(%ebp)
 8004ae2:	0f 85 58 fc ff ff    	jne    8004740 <vsnprintf+0x30>
 8004ae8:	eb 03                	jmp    8004aed <vsnprintf+0x3dd>
 8004aea:	90                   	nop
 8004aeb:	eb 01                	jmp    8004aee <vsnprintf+0x3de>
 8004aed:	90                   	nop
 8004aee:	8b 45 fc             	mov    -0x4(%ebp),%eax
 8004af1:	c6 00 00             	movb   $0x0,(%eax)
 8004af4:	8b 45 0c             	mov    0xc(%ebp),%eax
 8004af7:	2b 45 f4             	sub    -0xc(%ebp),%eax
 8004afa:	83 e8 01             	sub    $0x1,%eax
 8004afd:	c9                   	leave
 8004afe:	c3                   	ret

08004aff <__udivdi3>:
 8004aff:	55                   	push   %ebp
 8004b00:	89 e5                	mov    %esp,%ebp
 8004b02:	57                   	push   %edi
 8004b03:	56                   	push   %esi
 8004b04:	83 ec 30             	sub    $0x30,%esp
 8004b07:	8b 45 08             	mov    0x8(%ebp),%eax
 8004b0a:	89 45 d0             	mov    %eax,-0x30(%ebp)
 8004b0d:	8b 45 0c             	mov    0xc(%ebp),%eax
 8004b10:	89 45 d4             	mov    %eax,-0x2c(%ebp)
 8004b13:	8b 45 10             	mov    0x10(%ebp),%eax
 8004b16:	89 45 c8             	mov    %eax,-0x38(%ebp)
 8004b19:	8b 45 14             	mov    0x14(%ebp),%eax
 8004b1c:	89 45 cc             	mov    %eax,-0x34(%ebp)
 8004b1f:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8004b22:	0b 45 cc             	or     -0x34(%ebp),%eax
 8004b25:	75 0f                	jne    8004b36 <__udivdi3+0x37>
 8004b27:	b8 00 00 00 00       	mov    $0x0,%eax
 8004b2c:	ba 00 00 00 00       	mov    $0x0,%edx
 8004b31:	e9 ab 00 00 00       	jmp    8004be1 <__udivdi3+0xe2>
 8004b36:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8004b3d:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8004b44:	c7 45 e8 00 00 00 00 	movl   $0x0,-0x18(%ebp)
 8004b4b:	c7 45 ec 00 00 00 00 	movl   $0x0,-0x14(%ebp)
 8004b52:	c7 45 e4 3f 00 00 00 	movl   $0x3f,-0x1c(%ebp)
 8004b59:	eb 7a                	jmp    8004bd5 <__udivdi3+0xd6>
 8004b5b:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8004b5e:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8004b61:	0f a4 c2 01          	shld   $0x1,%eax,%edx
 8004b65:	01 c0                	add    %eax,%eax
 8004b67:	89 c6                	mov    %eax,%esi
 8004b69:	89 d7                	mov    %edx,%edi
 8004b6b:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
 8004b6e:	8b 45 d0             	mov    -0x30(%ebp),%eax
 8004b71:	8b 55 d4             	mov    -0x2c(%ebp),%edx
 8004b74:	0f ad d0             	shrd   %cl,%edx,%eax
 8004b77:	d3 ea                	shr    %cl,%edx
 8004b79:	f6 c1 20             	test   $0x20,%cl
 8004b7c:	74 04                	je     8004b82 <__udivdi3+0x83>
 8004b7e:	89 d0                	mov    %edx,%eax
 8004b80:	31 d2                	xor    %edx,%edx
 8004b82:	83 e0 01             	and    $0x1,%eax
 8004b85:	ba 00 00 00 00       	mov    $0x0,%edx
 8004b8a:	09 f0                	or     %esi,%eax
 8004b8c:	09 fa                	or     %edi,%edx
 8004b8e:	89 45 e8             	mov    %eax,-0x18(%ebp)
 8004b91:	89 55 ec             	mov    %edx,-0x14(%ebp)
 8004b94:	8b 45 e8             	mov    -0x18(%ebp),%eax
 8004b97:	8b 55 ec             	mov    -0x14(%ebp),%edx
 8004b9a:	3b 45 c8             	cmp    -0x38(%ebp),%eax
 8004b9d:	89 d0                	mov    %edx,%eax
 8004b9f:	1b 45 cc             	sbb    -0x34(%ebp),%eax
 8004ba2:	72 2d                	jb     8004bd1 <__udivdi3+0xd2>
 8004ba4:	8b 45 c8             	mov    -0x38(%ebp),%eax
 8004ba7:	8b 55 cc             	mov    -0x34(%ebp),%edx
 8004baa:	29 45 e8             	sub    %eax,-0x18(%ebp)
 8004bad:	19 55 ec             	sbb    %edx,-0x14(%ebp)
 8004bb0:	8b 4d e4             	mov    -0x1c(%ebp),%ecx
 8004bb3:	b8 01 00 00 00       	mov    $0x1,%eax
 8004bb8:	ba 00 00 00 00       	mov    $0x0,%edx
 8004bbd:	0f a5 c2             	shld   %cl,%eax,%edx
 8004bc0:	d3 e0                	shl    %cl,%eax
 8004bc2:	f6 c1 20             	test   $0x20,%cl
 8004bc5:	74 04                	je     8004bcb <__udivdi3+0xcc>
 8004bc7:	89 c2                	mov    %eax,%edx
 8004bc9:	31 c0                	xor    %eax,%eax
 8004bcb:	09 45 f0             	or     %eax,-0x10(%ebp)
 8004bce:	09 55 f4             	or     %edx,-0xc(%ebp)
 8004bd1:	83 6d e4 01          	subl   $0x1,-0x1c(%ebp)
 8004bd5:	83 7d e4 00          	cmpl   $0x0,-0x1c(%ebp)
 8004bd9:	79 80                	jns    8004b5b <__udivdi3+0x5c>
 8004bdb:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8004bde:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8004be1:	83 c4 30             	add    $0x30,%esp
 8004be4:	5e                   	pop    %esi
 8004be5:	5f                   	pop    %edi
 8004be6:	5d                   	pop    %ebp
 8004be7:	c3                   	ret

08004be8 <__umoddi3>:
 8004be8:	55                   	push   %ebp
 8004be9:	89 e5                	mov    %esp,%ebp
 8004beb:	57                   	push   %edi
 8004bec:	56                   	push   %esi
 8004bed:	83 ec 20             	sub    $0x20,%esp
 8004bf0:	8b 45 08             	mov    0x8(%ebp),%eax
 8004bf3:	89 45 e0             	mov    %eax,-0x20(%ebp)
 8004bf6:	8b 45 0c             	mov    0xc(%ebp),%eax
 8004bf9:	89 45 e4             	mov    %eax,-0x1c(%ebp)
 8004bfc:	8b 45 10             	mov    0x10(%ebp),%eax
 8004bff:	89 45 d8             	mov    %eax,-0x28(%ebp)
 8004c02:	8b 45 14             	mov    0x14(%ebp),%eax
 8004c05:	89 45 dc             	mov    %eax,-0x24(%ebp)
 8004c08:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8004c0b:	0b 45 dc             	or     -0x24(%ebp),%eax
 8004c0e:	75 0c                	jne    8004c1c <__umoddi3+0x34>
 8004c10:	b8 00 00 00 00       	mov    $0x0,%eax
 8004c15:	ba 00 00 00 00       	mov    $0x0,%edx
 8004c1a:	eb 7c                	jmp    8004c98 <__umoddi3+0xb0>
 8004c1c:	c7 45 f0 00 00 00 00 	movl   $0x0,-0x10(%ebp)
 8004c23:	c7 45 f4 00 00 00 00 	movl   $0x0,-0xc(%ebp)
 8004c2a:	c7 45 ec 3f 00 00 00 	movl   $0x3f,-0x14(%ebp)
 8004c31:	eb 59                	jmp    8004c8c <__umoddi3+0xa4>
 8004c33:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8004c36:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8004c39:	0f a4 c2 01          	shld   $0x1,%eax,%edx
 8004c3d:	01 c0                	add    %eax,%eax
 8004c3f:	89 c6                	mov    %eax,%esi
 8004c41:	89 d7                	mov    %edx,%edi
 8004c43:	8b 4d ec             	mov    -0x14(%ebp),%ecx
 8004c46:	8b 45 e0             	mov    -0x20(%ebp),%eax
 8004c49:	8b 55 e4             	mov    -0x1c(%ebp),%edx
 8004c4c:	0f ad d0             	shrd   %cl,%edx,%eax
 8004c4f:	d3 ea                	shr    %cl,%edx
 8004c51:	f6 c1 20             	test   $0x20,%cl
 8004c54:	74 04                	je     8004c5a <__umoddi3+0x72>
 8004c56:	89 d0                	mov    %edx,%eax
 8004c58:	31 d2                	xor    %edx,%edx
 8004c5a:	83 e0 01             	and    $0x1,%eax
 8004c5d:	ba 00 00 00 00       	mov    $0x0,%edx
 8004c62:	09 f0                	or     %esi,%eax
 8004c64:	09 fa                	or     %edi,%edx
 8004c66:	89 45 f0             	mov    %eax,-0x10(%ebp)
 8004c69:	89 55 f4             	mov    %edx,-0xc(%ebp)
 8004c6c:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8004c6f:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8004c72:	3b 45 d8             	cmp    -0x28(%ebp),%eax
 8004c75:	89 d0                	mov    %edx,%eax
 8004c77:	1b 45 dc             	sbb    -0x24(%ebp),%eax
 8004c7a:	72 0c                	jb     8004c88 <__umoddi3+0xa0>
 8004c7c:	8b 45 d8             	mov    -0x28(%ebp),%eax
 8004c7f:	8b 55 dc             	mov    -0x24(%ebp),%edx
 8004c82:	29 45 f0             	sub    %eax,-0x10(%ebp)
 8004c85:	19 55 f4             	sbb    %edx,-0xc(%ebp)
 8004c88:	83 6d ec 01          	subl   $0x1,-0x14(%ebp)
 8004c8c:	83 7d ec 00          	cmpl   $0x0,-0x14(%ebp)
 8004c90:	79 a1                	jns    8004c33 <__umoddi3+0x4b>
 8004c92:	8b 45 f0             	mov    -0x10(%ebp),%eax
 8004c95:	8b 55 f4             	mov    -0xc(%ebp),%edx
 8004c98:	83 c4 20             	add    $0x20,%esp
 8004c9b:	5e                   	pop    %esi
 8004c9c:	5f                   	pop    %edi
 8004c9d:	5d                   	pop    %ebp
 8004c9e:	c3                   	ret
