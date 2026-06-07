; ModuleID = 'loop_complex.O0.ll'
source_filename = "loop_complex.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define void @fun(i32 noundef %a, i32 noundef %b) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc20, %entry
  %i.0 = phi i32 [ 1, %entry ], [ %inc21, %for.inc20 ]
  %cmp = icmp slt i32 %i.0, 5
  br i1 %cmp, label %for.body, label %for.end22

for.body:                                         ; preds = %for.cond
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %j.0 = phi i32 [ 0, %for.body ], [ %inc, %for.inc ]
  %cmp2 = icmp slt i32 %j.0, 5
  br i1 %cmp2, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %j.0)
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %inc = add nsw i32 %j.0, 1
  br label %for.cond1, !llvm.loop !6

for.end:                                          ; preds = %for.cond1
  br label %for.cond5

for.cond5:                                        ; preds = %for.inc9, %for.end
  %j4.0 = phi i32 [ 0, %for.end ], [ %inc10, %for.inc9 ]
  %cmp6 = icmp slt i32 %j4.0, 5
  br i1 %cmp6, label %for.body7, label %for.end11

for.body7:                                        ; preds = %for.cond5
  %call8 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %j4.0)
  br label %for.inc9

for.inc9:                                         ; preds = %for.body7
  %inc10 = add nsw i32 %j4.0, 1
  br label %for.cond5, !llvm.loop !8

for.end11:                                        ; preds = %for.cond5
  br label %for.cond13

for.cond13:                                       ; preds = %for.inc17, %for.end11
  %j12.0 = phi i32 [ 5, %for.end11 ], [ %inc18, %for.inc17 ]
  %cmp14 = icmp slt i32 %j12.0, 10
  br i1 %cmp14, label %for.body15, label %for.end19

for.body15:                                       ; preds = %for.cond13
  %call16 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %j12.0)
  br label %for.inc17

for.inc17:                                        ; preds = %for.body15
  %inc18 = add nsw i32 %j12.0, 1
  br label %for.cond13, !llvm.loop !9

for.end19:                                        ; preds = %for.cond13
  br label %for.inc20

for.inc20:                                        ; preds = %for.end19
  %inc21 = add nsw i32 %i.0, 1
  br label %for.cond, !llvm.loop !10

for.end22:                                        ; preds = %for.cond
  br label %for.cond23

for.cond23:                                       ; preds = %for.inc27, %for.end22
  %z.0 = phi i32 [ 0, %for.end22 ], [ %inc28, %for.inc27 ]
  %cmp24 = icmp slt i32 %z.0, 4
  br i1 %cmp24, label %for.body25, label %for.end29

for.body25:                                       ; preds = %for.cond23
  %call26 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %z.0)
  br label %for.inc27

for.inc27:                                        ; preds = %for.body25
  %inc28 = add nsw i32 %z.0, 1
  br label %for.cond23, !llvm.loop !11

for.end29:                                        ; preds = %for.cond23
  br label %for.cond31

for.cond31:                                       ; preds = %for.inc59, %for.end29
  %i30.0 = phi i32 [ 0, %for.end29 ], [ %inc60, %for.inc59 ]
  %cmp32 = icmp slt i32 %i30.0, 4
  br i1 %cmp32, label %for.body33, label %for.end61

for.body33:                                       ; preds = %for.cond31
  br label %for.cond34

for.cond34:                                       ; preds = %for.inc56, %for.body33
  %k.0 = phi i32 [ 0, %for.body33 ], [ %inc57, %for.inc56 ]
  %cmp35 = icmp slt i32 %k.0, 4
  br i1 %cmp35, label %for.body36, label %for.end58

for.body36:                                       ; preds = %for.cond34
  %cmp37 = icmp slt i32 16, 44
  br i1 %cmp37, label %if.then, label %if.end

if.then:                                          ; preds = %for.body36
  br label %for.cond38

for.cond38:                                       ; preds = %for.inc42, %if.then
  %m.0 = phi i32 [ 0, %if.then ], [ %inc43, %for.inc42 ]
  %cmp39 = icmp slt i32 %m.0, 4
  br i1 %cmp39, label %for.body40, label %for.end44

for.body40:                                       ; preds = %for.cond38
  %call41 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %m.0)
  br label %for.inc42

for.inc42:                                        ; preds = %for.body40
  %inc43 = add nsw i32 %m.0, 1
  br label %for.cond38, !llvm.loop !12

for.end44:                                        ; preds = %for.cond38
  br label %if.end

if.end:                                           ; preds = %for.end44, %for.body36
  %cmp45 = icmp slt i32 16, 44
  br i1 %cmp45, label %if.then46, label %if.end55

if.then46:                                        ; preds = %if.end
  br label %for.cond48

for.cond48:                                       ; preds = %for.inc52, %if.then46
  %m47.0 = phi i32 [ 0, %if.then46 ], [ %inc53, %for.inc52 ]
  %cmp49 = icmp slt i32 %m47.0, 4
  br i1 %cmp49, label %for.body50, label %for.end54

for.body50:                                       ; preds = %for.cond48
  %call51 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %m47.0)
  br label %for.inc52

for.inc52:                                        ; preds = %for.body50
  %inc53 = add nsw i32 %m47.0, 1
  br label %for.cond48, !llvm.loop !13

for.end54:                                        ; preds = %for.cond48
  br label %if.end55

if.end55:                                         ; preds = %for.end54, %if.end
  br label %for.inc56

for.inc56:                                        ; preds = %if.end55
  %inc57 = add nsw i32 %k.0, 1
  br label %for.cond34, !llvm.loop !14

for.end58:                                        ; preds = %for.cond34
  br label %for.inc59

for.inc59:                                        ; preds = %for.end58
  %inc60 = add nsw i32 %i30.0, 1
  br label %for.cond31, !llvm.loop !15

for.end61:                                        ; preds = %for.cond31
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
!12 = distinct !{!12, !7}
!13 = distinct !{!13, !7}
!14 = distinct !{!14, !7}
!15 = distinct !{!15, !7}
