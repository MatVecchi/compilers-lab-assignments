; ModuleID = 'str_red.O0.ll'
source_filename = "test_str_red.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = shl i32 %0, 1
  %4 = shl i32 %0, 1
  %5 = add i32 %0, %4
  %6 = shl i32 %0, 1
  %7 = shl i32 %0, 2
  %8 = add i32 %7, %6
  %9 = shl i32 %0, 3
  %10 = add i32 %9, %8
  %11 = shl i32 %0, 4
  %12 = add i32 %11, %10
  %13 = add i32 %0, %12
  %14 = zext i32 %0 to i64
  %15 = zext i32 -858993459 to i64
  %16 = mul i64 %14, %15
  %17 = lshr i64 %16, 32
  %18 = trunc i64 %17 to i32
  %19 = lshr i32 %18, 3
  %20 = ashr i32 %1, 2
  %21 = shl i32 %1, 1
  %22 = shl i32 %1, 2
  %23 = add i32 %22, %21
  %24 = shl i32 %1, 3
  %25 = add i32 %24, %23
  %26 = add i32 %1, %25
  %27 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %3)
  %28 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %5)
  %29 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %20)
  %30 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %1)
  %31 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %13)
  %32 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef 0)
  %33 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %19)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+altnzcv,+ccdp,+ccidx,+complxnum,+crc,+dit,+dotprod,+flagm,+fp-armv8,+fp16fml,+fptoint,+fullfp16,+jsconv,+lse,+neon,+pauth,+perfmon,+predres,+ras,+rcpc,+rdm,+sb,+sha2,+sha3,+specrestrict,+ssbs,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8a,+zcm,+zcz" }
attributes #1 = { "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+altnzcv,+ccdp,+ccidx,+complxnum,+crc,+dit,+dotprod,+flagm,+fp-armv8,+fp16fml,+fptoint,+fullfp16,+jsconv,+lse,+neon,+pauth,+perfmon,+predres,+ras,+rcpc,+rdm,+sb,+sha2,+sha3,+specrestrict,+ssbs,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 26, i32 4]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 19.1.7 (/Users/runner/work/llvm-project/llvm-project/clang cd708029e0b2869e80abe31ddb175f7c35361f90)"}
