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
  %g.0 = phi i32 [ 0, %entry ], [ %g.1, %while.end ]
  %e.addr.0 = phi i32 [ %e, %entry ], [ %e.addr.1, %while.end ]
  %c.addr.0 = phi i32 [ %c, %entry ], [ %inc, %while.end ]
  %cmp = icmp slt i32 %c.addr.0, 10
  br i1 %cmp, label %while.body, label %while.end9

while.body:                                       ; preds = %while.cond
  %add = add nsw i32 5, 5
  %add1 = add nsw i32 %add, %c.addr.0
  %add2 = add nsw i32 %g.0, 1
  %cmp3 = icmp eq i32 %c.addr.0, 5
  br i1 %cmp3, label %if.then, label %if.end

if.then:                                          ; preds = %while.body
  br label %if.end

if.end:                                           ; preds = %if.then, %while.body
  %g.1 = phi i32 [ %add, %if.then ], [ %add2, %while.body ]
  %inc = add nsw i32 %c.addr.0, 1
  br label %while.cond4

while.cond4:                                      ; preds = %while.body6, %if.end
  %e.addr.1 = phi i32 [ %e.addr.0, %if.end ], [ %inc8, %while.body6 ]
  %cmp5 = icmp slt i32 %e.addr.1, 10
  br i1 %cmp5, label %while.body6, label %while.end

while.body6:                                      ; preds = %while.cond4
  %add7 = add nsw i32 5, 10
  %inc8 = add nsw i32 %e.addr.1, 1
  br label %while.cond4, !llvm.loop !6

while.end:                                        ; preds = %while.cond4
  br label %while.cond, !llvm.loop !8

while.end9:                                       ; preds = %while.cond
  br label %while.cond10

while.cond10:                                     ; preds = %while.body12, %while.end9
  %d.1 = phi i32 [ %d.0, %while.end9 ], [ %add13, %while.body12 ]
  %cmp11 = icmp slt i32 %c.addr.0, 20
  br i1 %cmp11, label %while.body12, label %while.end14

while.body12:                                     ; preds = %while.cond10
  %add13 = add nsw i32 5, 6
  br label %while.cond10, !llvm.loop !9

while.end14:                                      ; preds = %while.cond10
  %add15 = add nsw i32 %d.1, 1
  %add16 = add nsw i32 %g.0, 1
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
!9 = distinct !{!9, !7}
