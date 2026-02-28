; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @fibonacci(i64 %0) {
entry:
  %n = load i64, i64 %0, align 4
  %letmp = icmp sle i64 %n, 1
  br i1 %letmp, label %then, label %merge

then:                                             ; preds = %entry
  %n1 = load i64, i64 %0, align 4
  ret i64 %n1

merge:                                            ; preds = %entry
  %a = alloca i64, align 8
  store i64 0, ptr %a, align 4
  %b = alloca i64, align 8
  store i64 1, ptr %b, align 4
  %i = alloca i64, align 8
  store i64 2, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %merge
  %i2 = load i64, ptr %i, align 4
  %n3 = load i64, i64 %0, align 4
  %letmp4 = icmp sle i64 %i2, %n3
  br i1 %letmp4, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %temp = alloca i64, align 8
  %a5 = load i64, ptr %a, align 4
  %b6 = load i64, ptr %b, align 4
  %addtmp = add i64 %a5, %b6
  store i64 %addtmp, ptr %temp, align 4
  %a7 = load i64, ptr %a, align 4
  %b8 = load i64, ptr %b, align 4
  %b9 = load i64, ptr %b, align 4
  %temp10 = load i64, ptr %temp, align 4
  %i11 = load i64, ptr %i, align 4
  %i12 = load i64, ptr %i, align 4
  %addtmp13 = add i64 %i12, 1
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %b14 = load i64, ptr %b, align 4
  ret i64 %b14
}
