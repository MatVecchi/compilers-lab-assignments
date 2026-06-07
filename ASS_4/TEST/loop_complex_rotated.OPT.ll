; ModuleID = 'loop_complex_rotated.O0.ll'
source_filename = "loop_complex.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %a, i32 noundef %b) #0 {
entry:
  br label %for.body

for.body:                                         ; preds = %for.inc20, %entry
  %i.04 = phi i32 [ 1, %entry ], [ %inc21, %for.inc20 ]
  %fused.iv1 = add i32 %i.04, -1
  %fused.iv = add i32 %i.04, -1
  br label %for.body3

for.body3:                                        ; preds = %for.inc, %for.body
  %j.01 = phi i32 [ 0, %for.body ], [ %inc, %for.inc ]
  %fused.iv3 = add i32 %j.01, 5
  %fused.iv2 = add i32 %j.01, 0
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %j.01)
  br label %for.body7

for.inc:                                          ; preds = %for.body15
  %inc = add nsw i32 %j.01, 1
  %cmp2 = icmp slt i32 %inc, 5
  br i1 %cmp2, label %for.body3, label %for.end19, !llvm.loop !6

for.body7:                                        ; preds = %for.body3
  %call8 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %fused.iv2)
  br label %for.body15

for.body15:                                       ; preds = %for.body7
  %call16 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %fused.iv3)
  br label %for.inc

for.end19:                                        ; preds = %for.inc
  br label %for.body25

for.inc20:                                        ; preds = %for.end58
  %inc21 = add nsw i32 %i.04, 1
  %cmp = icmp slt i32 %inc21, 5
  br i1 %cmp, label %for.body, label %for.end61, !llvm.loop !8

for.body25:                                       ; preds = %for.end19
  %call26 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %fused.iv)
  br label %for.body33

for.body33:                                       ; preds = %for.body25
  br label %for.body36

for.body36:                                       ; preds = %for.inc56, %for.body33
  %k.08 = phi i32 [ 0, %for.body33 ], [ %inc57, %for.inc56 ]
  %cmp37 = icmp slt i32 16, 44
  br i1 %cmp37, label %if.then, label %if.end55

if.then:                                          ; preds = %for.body36
  br label %for.body40

for.body40:                                       ; preds = %for.inc42, %if.then
  %m.06 = phi i32 [ 0, %if.then ], [ %inc43, %for.inc42 ]
  %fused.iv4 = add i32 %m.06, 0
  %call41 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %m.06)
  br label %for.body50

for.inc42:                                        ; preds = %for.body50
  %inc43 = add nsw i32 %m.06, 1
  %cmp39 = icmp slt i32 %inc43, 4
  br i1 %cmp39, label %for.body40, label %for.end54, !llvm.loop !9

for.body50:                                       ; preds = %for.body40
  %call51 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %fused.iv4)
  br label %for.inc42

for.end54:                                        ; preds = %for.inc42
  br label %if.end55

if.end55:                                         ; preds = %for.body36, %for.end54
  br label %for.inc56

for.inc56:                                        ; preds = %if.end55
  %inc57 = add nsw i32 %k.08, 1
  %cmp35 = icmp slt i32 %inc57, 4
  br i1 %cmp35, label %for.body36, label %for.end58, !llvm.loop !10

for.end58:                                        ; preds = %for.inc56
  br label %for.inc20

for.end61:                                        ; preds = %for.inc20
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
