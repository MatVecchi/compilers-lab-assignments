; ModuleID = 'loop_fusion.O0.ll'
source_filename = "loop_fusion.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %d, i32 noundef %k) #0 {
entry:
  %a = alloca [10 x i32], align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp slt i32 %i.0, 16
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %i.0)
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i.0, 1
  br label %for.cond, !llvm.loop !6

for.end:                                          ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc5, %for.end
  %j.0 = phi i32 [ 0, %for.end ], [ %inc6, %for.inc5 ]
  %cmp2 = icmp slt i32 %j.0, 16
  br i1 %cmp2, label %for.body3, label %for.end7

for.body3:                                        ; preds = %for.cond1
  %call4 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %j.0)
  br label %for.inc5

for.inc5:                                         ; preds = %for.body3
  %inc6 = add nsw i32 %j.0, 1
  br label %for.cond1, !llvm.loop !8

for.end7:                                         ; preds = %for.cond1
  %cmp8 = icmp slt i32 5, 16
  br i1 %cmp8, label %if.then, label %if.end

if.then:                                          ; preds = %for.end7
  br label %for.cond10

for.cond10:                                       ; preds = %for.inc14, %if.then
  %i9.0 = phi i32 [ 1, %if.then ], [ %inc15, %for.inc14 ]
  %cmp11 = icmp slt i32 %i9.0, 6
  br i1 %cmp11, label %for.body12, label %for.end16

for.body12:                                       ; preds = %for.cond10
  %call13 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %i9.0)
  %idxprom = sext i32 %i9.0 to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom
  store i32 0, ptr %arrayidx, align 4
  br label %for.inc14

for.inc14:                                        ; preds = %for.body12
  %inc15 = add nsw i32 %i9.0, 1
  br label %for.cond10, !llvm.loop !9

for.end16:                                        ; preds = %for.cond10
  br label %if.end

if.end:                                           ; preds = %for.end16, %for.end7
  %cmp17 = icmp slt i32 5, 32
  br i1 %cmp17, label %if.then18, label %if.end31

if.then18:                                        ; preds = %if.end
  br label %for.cond20

for.cond20:                                       ; preds = %for.inc28, %if.then18
  %j19.0 = phi i32 [ 0, %if.then18 ], [ %inc29, %for.inc28 ]
  %cmp21 = icmp slt i32 %j19.0, 5
  br i1 %cmp21, label %for.body22, label %for.end30

for.body22:                                       ; preds = %for.cond20
  %call23 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %j19.0)
  %idxprom24 = sext i32 %j19.0 to i64
  %arrayidx25 = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom24
  store i32 44, ptr %arrayidx25, align 4
  %idxprom26 = sext i32 %j19.0 to i64
  %arrayidx27 = getelementptr inbounds [10 x i32], ptr %a, i64 0, i64 %idxprom26
  %0 = load i32, ptr %arrayidx27, align 4
  %add = add nsw i32 %0, 1
  br label %for.inc28

for.inc28:                                        ; preds = %for.body22
  %inc29 = add nsw i32 %j19.0, 1
  br label %for.cond20, !llvm.loop !10

for.end30:                                        ; preds = %for.cond20
  br label %if.end31

if.end31:                                         ; preds = %for.end30, %if.end
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
