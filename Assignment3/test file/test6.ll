; ModuleID = 'test4.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %N = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, i32* %1
  store i32 -2, i32* %a, align 4
  store i32 5, i32* %b, align 4
  store i32 0, i32* %x, align 4
  store i32 100, i32* %N, align 4
  store i32 0, i32* %i, align 4
  br label %2

; <label>:2                                       ; preds = %21, %0
  %3 = load i32* %i, align 4
  %4 = add nsw i32 %3, 1
  store i32 %4, i32* %i, align 4
  %5 = load i32* %N, align 4
  %6 = icmp slt i32 %3, %5
  br i1 %6, label %7, label %22

; <label>:7                                       ; preds = %2
  %8 = load i32* %a, align 4
  %9 = icmp sgt i32 %8, 0
  br i1 %9, label %10, label %13

; <label>:10                                      ; preds = %7
  %11 = load i32* %x, align 4
  %12 = add nsw i32 %11, 7
  store i32 %12, i32* %x, align 4
  store i32 5, i32* %y, align 4
  br label %16

; <label>:13                                      ; preds = %7
  %14 = load i32* %x, align 4
  %15 = sub nsw i32 %14, 2
  store i32 %15, i32* %x, align 4
  store i32 1, i32* %y, align 4
  br label %16

; <label>:16                                      ; preds = %13, %10
  %17 = load i32* %b, align 4
  %18 = icmp sgt i32 %17, 0
  br i1 %18, label %19, label %20

; <label>:19                                      ; preds = %16
  store i32 6, i32* %a, align 4
  br label %21

; <label>:20                                      ; preds = %16
  store i32 -5, i32* %a, align 4
  br label %21

; <label>:21                                      ; preds = %20, %19
  br label %2

; <label>:22                                      ; preds = %2
  %23 = load i32* %1
  ret i32 %23
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.5.2-3ubuntu1 (tags/RELEASE_352/final) (based on LLVM 3.5.2)"}
