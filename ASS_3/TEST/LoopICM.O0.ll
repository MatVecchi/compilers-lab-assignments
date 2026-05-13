; ModuleID = 'LoopICM.O0.ll'
source_filename = "LoopICM.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @loop(i32 noundef %b, i32 noundef %c, i32 noundef %e) #0 {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.end, %entry
  %d.0 = phi i32 [ undef, %entry ], [ %add, %while.end ]
  %g.0 = phi i32 [ 0, %entry ], [ %add2, %while.end ]
  %e.addr.0 = phi i32 [ %e, %entry ], [ %e.addr.1, %while.end ]
  %c.addr.0 = phi i32 [ %c, %entry ], [ %inc, %while.end ]
  %cmp = icmp slt i32 %c.addr.0, 10
  br i1 %cmp, label %while.body, label %while.end8

while.body:                                       ; preds = %while.cond
  %add = add nsw i32 5, 5
  %add1 = add nsw i32 %add, %c.addr.0
  %add2 = add nsw i32 %g.0, 1
  %inc = add nsw i32 %c.addr.0, 1
  br label %while.cond3

while.cond3:                                      ; preds = %while.body5, %while.body
  %e.addr.1 = phi i32 [ %e.addr.0, %while.body ], [ %inc7, %while.body5 ]
  %cmp4 = icmp slt i32 %e.addr.1, 10
  br i1 %cmp4, label %while.body5, label %while.end

while.body5:                                      ; preds = %while.cond3
  %add6 = add nsw i32 5, 10
  %inc7 = add nsw i32 %e.addr.1, 1
  br label %while.cond3, !llvm.loop !6

while.end:                                        ; preds = %while.cond3
  br label %while.cond, !llvm.loop !8

while.end8:                                       ; preds = %while.cond
  %add9 = add nsw i32 %d.0, 1
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
