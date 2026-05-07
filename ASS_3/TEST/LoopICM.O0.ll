; ModuleID = 'LoopICM.O0.ll'
source_filename = "LoopICM.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @loop(i32 noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  br label %4

4:                                                ; preds = %14, %3
  %.0 = phi i32 [ undef, %3 ], [ %.1, %14 ]
  %5 = srem i32 %0, 2
  %6 = icmp eq i32 %5, 0
  br i1 %6, label %7, label %11

7:                                                ; preds = %4
  %8 = icmp eq i32 %1, 7
  br i1 %8, label %9, label %10

9:                                                ; preds = %7
  br label %16

10:                                               ; preds = %7
  br label %14

11:                                               ; preds = %4
  %12 = add nsw i32 %0, %1
  %13 = add nsw i32 %12, 2
  br label %14

14:                                               ; preds = %11, %10
  %.1 = phi i32 [ %.0, %10 ], [ %12, %11 ]
  %15 = add nsw i32 %.1, 1
  br label %4

16:                                               ; preds = %9
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
