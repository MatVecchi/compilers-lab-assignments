; ModuleID = 'loop_fusion.O0.ll'
source_filename = "loop_fusion.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %d, i32 noundef %k) #0 {
entry:
  %b = alloca [10 x i32], align 4
  %cmp = icmp slt i32 %d, %k
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  br label %do.body

do.body:                                          ; preds = %do.cond, %if.then
  %i.0 = phi i32 [ 0, %if.then ], [ %inc, %do.cond ]
  %inc = add nsw i32 %i.0, 1
  br label %do.cond

do.cond:                                          ; preds = %do.body
  %cmp1 = icmp slt i32 %inc, %d
  br i1 %cmp1, label %do.body, label %do.end, !llvm.loop !6

do.end:                                           ; preds = %do.cond
  br label %if.end

if.end:                                           ; preds = %do.end, %entry
  %add = add nsw i32 %k, 1
  %sub = sub nsw i32 %add, 1
  %cmp2 = icmp slt i32 %d, %sub
  br i1 %cmp2, label %if.then3, label %if.end9

if.then3:                                         ; preds = %if.end
  br label %do.body4

do.body4:                                         ; preds = %do.cond6, %if.then3
  %j.0 = phi i32 [ 0, %if.then3 ], [ %inc5, %do.cond6 ]
  %inc5 = add nsw i32 %j.0, 1
  br label %do.cond6

do.cond6:                                         ; preds = %do.body4
  %cmp7 = icmp slt i32 %inc5, %d
  br i1 %cmp7, label %do.body4, label %do.end8, !llvm.loop !8

do.end8:                                          ; preds = %do.cond6
  br label %if.end9

if.end9:                                          ; preds = %do.end8, %if.end
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %if.end9
  %i10.0 = phi i32 [ 0, %if.end9 ], [ %inc12, %for.inc ]
  %cmp11 = icmp slt i32 %i10.0, %d
  br i1 %cmp11, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %idxprom = sext i32 %i10.0 to i64
  %arrayidx = getelementptr inbounds [10 x i32], ptr %b, i64 0, i64 %idxprom
  store i32 16, ptr %arrayidx, align 4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc12 = add nsw i32 %i10.0, 1
  br label %for.cond, !llvm.loop !9

for.end:                                          ; preds = %for.cond
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
