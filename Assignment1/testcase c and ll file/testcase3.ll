; ModuleID = 'testcase3.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 0, i32* %1
  store i32 0, i32* %x, align 4
  %2 = load i32* %a, align 4
  %3 = icmp sgt i32 %2, 0
  br i1 %3, label %4, label %7

; <label>:4                                       ; preds = %0
  %5 = load i32* %x, align 4
  %6 = sub nsw i32 %5, 5
  store i32 %6, i32* %x, align 4
  br label %8

; <label>:7                                       ; preds = %0
  store i32 2, i32* %x, align 4
  br label %8

; <label>:8                                       ; preds = %7, %4
  %9 = load i32* %b, align 4
  %10 = icmp sgt i32 %9, 0
  br i1 %10, label %11, label %12

; <label>:11                                      ; preds = %8
  store i32 3, i32* %x, align 4
  br label %15

; <label>:12                                      ; preds = %8
  %13 = load i32* %x, align 4
  %14 = add nsw i32 %13, 51
  store i32 %14, i32* %x, align 4
  br label %15

; <label>:15                                      ; preds = %12, %11
  %16 = load i32* %1
  ret i32 %16
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.5.2-3ubuntu1 (tags/RELEASE_352/final) (based on LLVM 3.5.2)"}
