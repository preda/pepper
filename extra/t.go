package main

import "fmt"

func main() {
	sum := "a"
	sum, x := 10, 2
	for i := 0; i < 10; i++ {
		sum += x
	}
	fmt.Println(sum)
}
