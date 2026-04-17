; ModuleID = 'str_red.O0.ll'
source_filename = "test_str_red.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = mul nsw i32 %0, 2
  %4 = mul nsw i32 %0, 3
  %5 = mul nsw i32 %0, 31
  %6 = mul nsw i32 0, %0
  %7 = sdiv i32 %0, 10
  %8 = sdiv i32 %1, 4
  %9 = mul nsw i32 15, %1
  %10 = sdiv i32 %1, 1
  %11 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %3)
  %12 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %4)
  %13 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %8)
  %14 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %10)
  %15 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %5)
  %16 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %6)
  %17 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %7)
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
