; ModuleID = 'loop_fusion.O0.ll'
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

for.body:                                         ; preds = %entry, %for.inc25
  %j2.04 = phi i32 [ 0, %entry ], [ %inc26, %for.inc25 ]
  %cmp3 = icmp slt i32 5, 10
  br i1 %cmp3, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  br label %for.body6

for.body6:                                        ; preds = %if.then, %for.inc
  %n.01 = phi i32 [ 0, %if.then ], [ %inc, %for.inc ]
  %idxprom = sext i32 0 to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom
  store i32 1, ptr %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body6
  %inc = add nsw i32 %n.01, 1
  %cmp5 = icmp slt i32 %inc, 10
  br i1 %cmp5, label %for.body6, label %for.end, !llvm.loop !6

for.end:                                          ; preds = %for.inc
  br label %if.end

if.end:                                           ; preds = %for.end, %for.body
  %cmp7 = icmp slt i32 5, 10
  br i1 %cmp7, label %if.then8, label %if.end18

if.then8:                                         ; preds = %if.end
  br label %for.body12

for.body12:                                       ; preds = %if.then8, %for.inc15
  %n9.02 = phi i32 [ 0, %if.then8 ], [ %inc16, %for.inc15 ]
  %idxprom13 = sext i32 0 to i64
  %arrayidx14 = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom13
  store i32 16, ptr %arrayidx14, align 4
  br label %for.inc15

for.inc15:                                        ; preds = %for.body12
  %inc16 = add nsw i32 %n9.02, 1
  %cmp11 = icmp slt i32 %inc16, 10
  br i1 %cmp11, label %for.body12, label %for.end17, !llvm.loop !8

for.end17:                                        ; preds = %for.inc15
  br label %if.end18

if.end18:                                         ; preds = %for.end17, %if.end
  br label %for.body21

for.body21:                                       ; preds = %if.end18, %for.inc22
  %z.03 = phi i32 [ 0, %if.end18 ], [ %inc23, %for.inc22 ]
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %z.03)
  br label %for.inc22

for.inc22:                                        ; preds = %for.body21
  %inc23 = add nsw i32 %z.03, 1
  %cmp20 = icmp slt i32 %inc23, 5
  br i1 %cmp20, label %for.body21, label %for.end24, !llvm.loop !9

for.end24:                                        ; preds = %for.inc22
  br label %for.inc25

for.inc25:                                        ; preds = %for.end24
  %inc26 = add nsw i32 %j2.04, 1
  %cmp = icmp slt i32 %inc26, 5
  br i1 %cmp, label %for.body, label %for.end27, !llvm.loop !10

for.end27:                                        ; preds = %for.inc25
  br label %for.body31

for.body31:                                       ; preds = %for.end27, %for.inc33
  %j28.05 = phi i32 [ 0, %for.end27 ], [ %inc34, %for.inc33 ]
  %call32 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %add1)
  br label %for.inc33

for.inc33:                                        ; preds = %for.body31
  %inc34 = add nsw i32 %j28.05, 1
  %cmp30 = icmp slt i32 %inc34, 5
  br i1 %cmp30, label %for.body31, label %for.end35, !llvm.loop !11

for.end35:                                        ; preds = %for.inc33
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
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
