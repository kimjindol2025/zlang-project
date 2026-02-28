; ModuleID = 'zlang_program'
source_filename = "zlang_program"

define i64 @loop_test() {
entry:
  %i = alloca i64, align 8
  store i64 0, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %i1 = load i64, ptr %i, align 4
  %lttmp = icmp slt i64 %i1, 10
  br i1 %lttmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %i2 = load i64, ptr %i, align 4
  %i3 = load i64, ptr %i, align 4
  %addtmp = add i64 %i3, 1
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %i4 = load i64, ptr %i, align 4
  ret i64 %i4
}
