; ModuleID = 'loop_fusion_rotate.O0.ll'
source_filename = "loop_fusion.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %d, i32 noundef %k) #0 {
entry:
  %a = alloca [10 x i32], align 4
  %b = alloca [10 x i32], align 4
  %add = add nsw i32 5, 100
  %add1 = add nsw i32 10, 5
  br label %for.body

for.body:                                         ; preds = %for.inc25, %entry
  %j2.04 = phi i32 [ 0, %entry ], [ %inc26, %for.inc25 ]
  %fused.iv = add i32 %j2.04, 0
  %cmp3 = icmp slt i32 5, 10
  br i1 %cmp3, label %if.then, label %if.end18

if.then:                                          ; preds = %for.body
  br label %for.body6

for.body6:                                        ; preds = %for.inc, %if.then
  %n.01 = phi i32 [ 0, %if.then ], [ %inc, %for.inc ]
  %fused.iv1 = add i32 %n.01, 0
  %idxprom = sext i32 0 to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom
  store i32 1, ptr %arrayidx, align 4
  br label %for.body12

for.inc:                                          ; preds = %for.body12
  %inc = add nsw i32 %n.01, 1
  %cmp5 = icmp slt i32 %inc, 10
  br i1 %cmp5, label %for.body6, label %for.end17, !llvm.loop !6

for.body12:                                       ; preds = %for.body6
  %idxprom13 = sext i32 0 to i64
  %arrayidx14 = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom13
  store i32 16, ptr %arrayidx14, align 4
  br label %for.inc

for.end17:                                        ; preds = %for.inc
  br label %if.end18

if.end18:                                         ; preds = %for.body, %for.end17
  br label %for.body21

for.body21:                                       ; preds = %for.inc22, %if.end18
  %z.03 = phi i32 [ 0, %if.end18 ], [ %inc23, %for.inc22 ]
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %z.03)
  br label %for.inc22

for.inc22:                                        ; preds = %for.body21
  %inc23 = add nsw i32 %z.03, 1
  %cmp20 = icmp slt i32 %inc23, 5
  br i1 %cmp20, label %for.body21, label %for.end24, !llvm.loop !8

for.end24:                                        ; preds = %for.inc22
  br label %for.body31

for.inc25:                                        ; preds = %for.body31
  %inc26 = add nsw i32 %j2.04, 1
  %cmp = icmp slt i32 %inc26, 5
  br i1 %cmp, label %for.body, label %for.end35, !llvm.loop !9

for.body31:                                       ; preds = %for.end24
  %call32 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %add1)
  br label %for.inc25

for.end35:                                        ; preds = %for.inc25
  %arrayidx36 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 3
  %0 = load i32, ptr %arrayidx36, align 4
  ret void
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
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
