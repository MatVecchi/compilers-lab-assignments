; ModuleID = 'loop_fusion.O0.ll'
source_filename = "loop_fusion.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %d, i32 noundef %k) #0 {
entry:
  %b = alloca [10 x i32], align 4
  %a17 = alloca [20 x i32], align 4
  %add = add nsw i32 5, 100
  %add1 = add nsw i32 10, 5
  %cmp = icmp slt i32 5, 10
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %if.then
  %i2.0 = phi i32 [ 0, %if.then ], [ %inc, %for.inc ]
  %cmp3 = icmp slt i32 %i2.0, 5
  br i1 %cmp3, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = sext i32 %i2.0 to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom
  store i32 44, ptr %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i2.0, 1
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
  br label %if.end

if.end:                                           ; preds = %for.end, %entry
  %cmp4 = icmp slt i32 5, %add1
  br i1 %cmp4, label %if.then5, label %if.end38

if.then5:                                         ; preds = %if.end
  br label %for.cond7

for.cond7:                                        ; preds = %for.inc35, %if.then5
  %i6.0 = phi i32 [ 1, %if.then5 ], [ %inc36, %for.inc35 ]
  %add8 = add nsw i32 5, 1
  %cmp9 = icmp slt i32 %i6.0, %add8
  br i1 %cmp9, label %for.body10, label %for.end37

for.body10:                                       ; preds = %for.cond7
  %sub = sub nsw i32 %i6.0, 1
  %idxprom11 = sext i32 %sub to i64
  %arrayidx12 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom11
  %0 = load i32, ptr %arrayidx12, align 4
  %add13 = add nsw i32 16, %0
  %sub14 = sub nsw i32 %i6.0, 1
  %idxprom15 = sext i32 %sub14 to i64
  %arrayidx16 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom15
  store i32 %add13, ptr %arrayidx16, align 4
  br label %for.cond18

for.cond18:                                       ; preds = %for.inc23, %for.body10
  %n.0 = phi i32 [ 0, %for.body10 ], [ %inc24, %for.inc23 ]
  %cmp19 = icmp slt i32 %n.0, 10
  br i1 %cmp19, label %for.body20, label %for.end25

for.body20:                                       ; preds = %for.cond18
  %idxprom21 = sext i32 %i6.0 to i64
  %arrayidx22 = getelementptr inbounds [20 x i32], ptr %a17, i64 0, i64 %idxprom21
  store i32 44, ptr %arrayidx22, align 4
  br label %for.inc23

for.inc23:                                        ; preds = %for.body20
  %inc24 = add nsw i32 %n.0, 1
  br label %for.cond18, !llvm.loop !8

for.end25:                                        ; preds = %for.cond18
  br label %for.cond27

for.cond27:                                       ; preds = %for.inc32, %for.end25
  %n26.0 = phi i32 [ 0, %for.end25 ], [ %inc33, %for.inc32 ]
  %cmp28 = icmp slt i32 %n26.0, 10
  br i1 %cmp28, label %for.body29, label %for.end34

for.body29:                                       ; preds = %for.cond27
  %idxprom30 = sext i32 %i6.0 to i64
  %arrayidx31 = getelementptr inbounds [20 x i32], ptr %a17, i64 0, i64 %idxprom30
  store i32 16, ptr %arrayidx31, align 4
  br label %for.inc32

for.inc32:                                        ; preds = %for.body29
  %inc33 = add nsw i32 %n26.0, 1
  br label %for.cond27, !llvm.loop !9

for.end34:                                        ; preds = %for.cond27
  br label %for.inc35

for.inc35:                                        ; preds = %for.end34
  %inc36 = add nsw i32 %i6.0, 1
  br label %for.cond7, !llvm.loop !10

for.end37:                                        ; preds = %for.cond7
  br label %if.end38

if.end38:                                         ; preds = %for.end37, %if.end
  %arrayidx39 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 3
  %1 = load i32, ptr %arrayidx39, align 4
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
!10 = distinct !{!10, !7}
