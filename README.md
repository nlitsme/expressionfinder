# expressionfinder
Tool for solving math problems involving finding an expression for a given value. (like numberphile's 10958 problem)

## background

The Numberphile video's
* (The 10,958 Problem - Numberphile)[https://www.youtube.com/watch?v=-ruC5A9EzzE]
* (A 10,958 Solution - Numberphile)[https://www.youtube.com/watch?v=pasyRUj7UwM]
* (Concatenation (extra footage) - Numberphile)[https://www.youtube.com/watch?v=LgnoYsbI7Uc]

Discussion
* https://www.reddit.com/r/BradyHaran/comments/6628q2/the_10958_problem_numberphile/
* https://www.reddit.com/r/videos/comments/663s6w/the_10958_problem_numberphile/
* https://puzzling.stackexchange.com/questions/51129/the-10-958-problem
* https://puzzling.stackexchange.com/questions/47923/rendering-the-number-10-958-with-the-string-1-2-3-4-5-6-7-8-9/47943

Similar tools
* https://github.com/basarane/10958-Problem---Random-Search/blob/master/random_search.py
* https://github.com/DaveJarvis/sequential

The papers discussed in the numberphile youtube video
* https://arxiv.org/pdf/1302.1479.pdf
  expressions for numbers 0 .. 11111  using 1..9  or 9..1
* https://arxiv.org/pdf/1502.03501.pdf
  expressions for numbers 0 .. 1111  using 1, 2, 3, 4, 5, 6, 7, 8 or 9

using addition, subtraction, multiplication, division, 'potentiation' ( == exponentiation ), concatenation, negation
where concatenation and negation are not explicitly mentioned

## findexpr

Findexpr is a tool which will generate all possible equations using the above operations and numbers, except for negation.

    findexpr -t 10958
    
Will take about 3 hours to search all, and report each which results in `10958`.

Just typing `findexpr` by itself, will report all values.

See the sourcecode for further explanation.

## Dependencies

* https://github.com/nlitsme/cpputils

