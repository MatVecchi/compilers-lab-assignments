; ModuleID = 'loop_fusion.OPT.ll'
source_filename = "loop_fusion.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %d, i32 noundef %k) #0 {
entry:
  %b = alloca [10 x i32], align 4
  %a9 = alloca [20 x i32], align 4
  %add = add nsw i32 5, 100
  %add1 = add nsw i32 10, 5
  %cmp = icmp slt i32 5, %add1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %for.cond

for.cond:                                         ; preds = %for.inc33, %if.then
  %i2.0 = phi i32 [ 1, %if.then ], [ %inc34, %for.inc33 ]
  %add3 = add nsw i32 5, 1
  %cmp4 = icmp slt i32 %i2.0, %add3
  br i1 %cmp4, label %for.body, label %for.end35

for.body:                                         ; preds = %for.cond
  %sub = sub nsw i32 %i2.0, 1
  %idxprom = sext i32 %sub to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom
  %0 = load i32, ptr %arrayidx, align 4
  %add5 = add nsw i32 16, %0
  %sub6 = sub nsw i32 %i2.0, 1
  %idxprom7 = sext i32 %sub6 to i64
  %arrayidx8 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom7
  store i32 %add5, ptr %arrayidx8, align 4
  br label %for.cond10

for.cond10:                                       ; preds = %for.inc, %for.body
  %n.0 = phi i32 [ 0, %for.body ], [ %inc, %for.inc ]
  %fused.iv1 = add i32 %n.0, 0
  %fused.iv = add i32 %n.0, 0
  %cmp11 = icmp slt i32 %n.0, 10
  br i1 %cmp11, label %for.body12, label %for.end32

for.body12:                                       ; preds = %for.cond10
  %idxprom13 = sext i32 %i2.0 to i64
  %arrayidx14 = getelementptr inbounds [20 x i32], ptr %a9, i64 0, i64 %idxprom13
  store i32 44, ptr %arrayidx14, align 4
  br label %for.body18

for.inc:                                          ; preds = %for.body27
  %inc = add nsw i32 %n.0, 1
  br label %for.cond10, !llvm.loop !6

for.body18:                                       ; preds = %for.body12
  %idxprom19 = sext i32 %i2.0 to i64
  %arrayidx20 = getelementptr inbounds [20 x i32], ptr %a9, i64 0, i64 %idxprom19
  store i32 55, ptr %arrayidx20, align 4
  br label %for.body27

for.body27:                                       ; preds = %for.body18
  %idxprom28 = sext i32 %i2.0 to i64
  %arrayidx29 = getelementptr inbounds [20 x i32], ptr %a9, i64 0, i64 %idxprom28
  store i32 16, ptr %arrayidx29, align 4
  br label %for.inc

for.end32:                                        ; preds = %for.cond10
  br label %for.inc33

for.inc33:                                        ; preds = %for.end32
  %inc34 = add nsw i32 %i2.0, 1
  br label %for.cond, !llvm.loop !8

for.end35:                                        ; preds = %for.cond
  br label %if.end

if.end:                                           ; preds = %for.end35, %entry
  %arrayidx36 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 3
  %1 = load i32, ptr %arrayidx36, align 4
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
