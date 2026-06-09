; ModuleID = 'str_red.O0.ll'
source_filename = "str_red.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @foo(i32 noundef %a, i32 noundef %b) #0 {
entry:
  %mul = mul nsw i32 %a, 2
  %mul1 = mul nsw i32 %a, 3
  %mul2 = mul nsw i32 %a, 31
  %mul3 = mul nsw i32 0, %a
  %div = sdiv i32 %a, 10
  %div4 = sdiv i32 %b, 4
  %mul5 = mul nsw i32 15, %b
  %div6 = sdiv i32 %b, 1
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul)
  %call7 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul1)
  %call8 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %div4)
  %call9 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %div6)
  %call10 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul2)
  %call11 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul3)
  %call12 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %div)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 19.1.7 (/home/runner/work/llvm-project/llvm-project/clang cd708029e0b2869e80abe31ddb175f7c35361f90)"}
