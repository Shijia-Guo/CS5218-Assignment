; ModuleID = 'testcase4.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 0, i32* %1
  %2 = load i32* %a, align 4
  %3 = icmp sgt i32 %2, 0
  br i1 %3, label %4, label %7

; <label>:4                                       ; preds = %0
  %5 = load i32* %c, align 4
  %6 = add nsw i32 %5, 10
  store i32 %6, i32* %c, align 4
  br label %15

; <label>:7                                       ; preds = %0
  %8 = load i32* %b, align 4
  %9 = icmp sgt i32 %8, 0
  br i1 %9, label %10, label %11

; <label>:10                                      ; preds = %7
  store i32 3, i32* %c, align 4
  br label %14

; <label>:11                                      ; preds = %7
  %12 = load i32* %c, align 4
  %13 = add nsw i32 %12, 51
  store i32 %13, i32* %c, align 4
  br label %14

; <label>:14                                      ; preds = %11, %10
  br label %15

; <label>:15                                      ; preds = %14, %4
  %16 = load i32* %d, align 4
  %17 = icmp sgt i32 %16, 0
  br i1 %17, label %18, label %19

; <label>:18                                      ; preds = %15
  store i32 3, i32* %x, align 4
  br label %22

; <label>:19                                      ; preds = %15
  %20 = load i32* %x, align 4
  %21 = add nsw i32 %20, 51
  store i32 %21, i32* %x, align 4
  br label %22

; <label>:22                                      ; preds = %19, %18
  %23 = load i32* %1
  ret i32 %23
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.5.2-3ubuntu1 (tags/RELEASE_352/final) (based on LLVM 3.5.2)"}
