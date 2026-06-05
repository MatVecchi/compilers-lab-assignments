; ModuleID = 'loop_fusion.O0.ll'
source_filename = "loop_fusion.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %d, i32 noundef %k) #0 {
entry:
  %b = alloca [10 x i32], align 4
  %add = add nsw i32 5, 100
  %cmp = icmp slt i32 5, 10
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %for.body

for.body:                                         ; preds = %if.then, %for.inc
  %i1.01 = phi i32 [ 0, %if.then ], [ %inc, %for.inc ]
  %idxprom = sext i32 %i1.01 to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom
  store i32 44, ptr %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add nsw i32 %i1.01, 1
  %cmp2 = icmp slt i32 %inc, 5
  br i1 %cmp2, label %for.body, label %for.end, !llvm.loop !6

for.end:                                          ; preds = %for.inc
  br label %if.end

if.end:                                           ; preds = %for.end, %entry
  %cmp3 = icmp slt i32 5, 10
  br i1 %cmp3, label %if.then4, label %if.end19

if.then4:                                         ; preds = %if.end
  %add7 = add nsw i32 5, 1
  br label %for.body9

for.body9:                                        ; preds = %if.then4, %for.inc16
  %i5.02 = phi i32 [ 1, %if.then4 ], [ %inc17, %for.inc16 ]
  %sub = sub nsw i32 %i5.02, 1
  %idxprom10 = sext i32 %sub to i64
  %arrayidx11 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom10
  %0 = load i32, ptr %arrayidx11, align 4
  %add12 = add nsw i32 16, %0
  %sub13 = sub nsw i32 %i5.02, 1
  %idxprom14 = sext i32 %sub13 to i64
  %arrayidx15 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom14
  store i32 %add12, ptr %arrayidx15, align 4
  br label %for.inc16

for.inc16:                                        ; preds = %for.body9
  %inc17 = add nsw i32 %i5.02, 1
  %cmp8 = icmp slt i32 %inc17, %add7
  br i1 %cmp8, label %for.body9, label %for.end18, !llvm.loop !8

for.end18:                                        ; preds = %for.inc16
  br label %if.end19

if.end19:                                         ; preds = %for.end18, %if.end
  %arrayidx20 = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 3
  %1 = load i32, ptr %arrayidx20, align 4
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
