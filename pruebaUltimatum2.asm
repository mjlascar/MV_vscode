mov edx, ds
sys %F
add edx, 123
mov cx, -1
sys %3
add edx, 3
sys %4
sys %7