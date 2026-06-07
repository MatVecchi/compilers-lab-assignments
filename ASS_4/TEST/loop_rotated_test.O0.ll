; ModuleID = 'loop_rotated_test.c'
source_filename = "loop_rotated_test.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun() #0 {
entry:
  %m = alloca i32, align 4
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  store i32 16, ptr %m, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %j, align 4
  br label %do.body

do.body:                                          ; preds = %do.cond, %entry
  %0 = load i32, ptr %i, align 4
  %inc = add nsw i32 %0, 1
  store i32 %inc, ptr %i, align 4
  br label %do.cond

do.cond:                                          ; preds = %do.body
  %1 = load i32, ptr %i, align 4
  %2 = load i32, ptr %m, align 4
  %cmp = icmp slt i32 %1, %2
  br i1 %cmp, label %do.body, label %do.end, !llvm.loop !6

do.end:                                           ; preds = %do.cond
  br label %do.body1

do.body1:                                         ; preds = %do.cond3, %do.end
  %3 = load i32, ptr %j, align 4
  %inc2 = add nsw i32 %3, 1
  store i32 %inc2, ptr %j, align 4
  br label %do.cond3

do.cond3:                                         ; preds = %do.body1
  %4 = load i32, ptr %j, align 4
  %5 = load i32, ptr %m, align 4
  %cmp4 = icmp slt i32 %4, %5
  br i1 %cmp4, label %do.body1, label %do.end5, !llvm.loop !8

do.end5:                                          ; preds = %do.cond3
  ret void
}

attributes #0 = { noinline nounwind ssp uwtable(sync) "frame-pointer"="non-leaf" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+altnzcv,+ccdp,+ccidx,+complxnum,+crc,+dit,+dotprod,+flagm,+fp-armv8,+fp16fml,+fptoint,+fullfp16,+jsconv,+lse,+neon,+pauth,+perfmon,+predres,+ras,+rcpc,+rdm,+sb,+sha2,+sha3,+specrestrict,+ssbs,+v8.1a,+v8.2a,+v8.3a,+v8.4a,+v8a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 26, i32 4]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 1}
!5 = !{!"clang version 19.1.7 (/Users/runner/work/llvm-project/llvm-project/clang cd708029e0b2869e80abe31ddb175f7c35361f90)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
