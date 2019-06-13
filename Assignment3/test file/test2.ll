; ModuleID = 'test2.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  %i = alloca i32, align 4
  %N = alloca i32, align 4
  store i32 0, i32* %1
  store i32 0, i32* %z, align 4
  store i32 0, i32* %i, align 4
  store i32 100, i32* %N, align 4
  br label %2

; <label>:2                                       ; preds = %6, %0
  %3 = load i32* %i, align 4
  %4 = load i32* %N, align 4
  %5 = icmp slt i32 %3, %4
  br i1 %5, label %6, label %26

; <label>:6                                       ; preds = %2
  %7 = load i32* %x, align 4
  %8 = load i32* %y, align 4
  %9 = mul nsw i32 2, %8
  %10 = mul nsw i32 %9, 3
  %11 = load i32* %z, align 4
  %12 = mul nsw i32 %10, %11
  %13 = add nsw i32 %7, %12
  %14 = srem i32 %13, 3
  %15 = sub nsw i32 0, %14
  store i32 %15, i32* %x, align 4
  %16 = load i32* %x, align 4
  %17 = mul nsw i32 3, %16
  %18 = load i32* %y, align 4
  %19 = mul nsw i32 2, %18
  %20 = add nsw i32 %17, %19
  %21 = load i32* %z, align 4
  %22 = add nsw i32 %20, %21
  %23 = srem i32 %22, 11
  store i32 %23, i32* %y, align 4
  %24 = load i32* %z, align 4
  %25 = add nsw i32 %24, 1
  store i32 %25, i32* %z, align 4
  br label %2

; <label>:26                                      ; preds = %2
  %27 = load i32* %1
  ret i32 %27
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.5.2-3ubuntu1 (tags/RELEASE_352/final) (based on LLVM 3.5.2)"}
