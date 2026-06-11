; ModuleID = 'str_red.O0.ll'
source_filename = "str_red.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-macosx16.0.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1

; Function Attrs: noinline nounwind ssp uwtable(sync)
define i32 @foo(i32 noundef %a, i32 noundef %b) #0 {
entry:
  %mul = mul nsw i32 %a, 2
  %mul1 = mul nsw i32 %a, 3
  %mul2 = mul nsw i32 %a, 31
  %mul3 = mul nsw i32 0, %a
  %div = sdiv i32 %a, 10
  %div4 = sdiv i32 %b, 4
  %mul5 = mul nsw i32 15, %b
  %div6 = sdiv i32 %b, 1
  %call = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul)
  %call7 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul1)
  %call8 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %div4)
  %call9 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %div6)
  %call10 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul2)
  %call11 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %mul3)
  %call12 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %div)
  ret i32 0
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
