; ModuleID = 'LoopICM.O0.ll'
source_filename = "LoopICM.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @loop(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = srem i32 %0, 2
  %5 = icmp eq i32 %4, 0
  %6 = add nsw i32 %0, %1
  %7 = add nsw i32 %6, 2
  %8 = icmp eq i32 %1, 7
  br label %9

9:                                                ; preds = %14, %3
  %.0 = phi i32 [ undef, %3 ], [ %.1, %14 ]
  br i1 %5, label %10, label %13

10:                                               ; preds = %9
  br i1 %8, label %11, label %12

11:                                               ; preds = %10
  br label %16

12:                                               ; preds = %10
  br label %14

13:                                               ; preds = %9
  br label %14

14:                                               ; preds = %13, %12
  %.1 = phi i32 [ %.0, %12 ], [ %6, %13 ]
  %15 = add nsw i32 %.1, 1
  br label %9

16:                                               ; preds = %11
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
