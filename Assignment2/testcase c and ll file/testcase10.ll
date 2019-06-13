; ModuleID = 'testcase10.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %sink = alloca i32, align 4
  %source = alloca i32, align 4
  store i32 0, i32* %1
  %2 = load i32* %source, align 4
  store i32 %2, i32* %b, align 4
  %3 = load i32* %a, align 4
  %4 = icmp sgt i32 %3, 0
  br i1 %4, label %5, label %7

; <label>:5                                       ; preds = %0
  %6 = load i32* %source, align 4
  store i32 %6, i32* %a, align 4
  br label %13

; <label>:7                                       ; preds = %0
  %8 = load i32* %c, align 4
  %9 = load i32* %d, align 4
  %10 = load i32* %sink, align 4
  %11 = sdiv i32 %9, %10
  %12 = sub nsw i32 %8, %11
  store i32 %12, i32* %source, align 4
  br label %13

; <label>:13                                      ; preds = %7, %5
  %14 = load i32* %a, align 4
  store i32 %14, i32* %c, align 4
  %15 = load i32* %1
  ret i32 %15
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"Ubuntu clang version 3.5.2-3ubuntu1 (tags/RELEASE_352/final) (based on LLVM 3.5.2)"}
