### Pure Ugly Lambda

An interpreter of the lambda calculus using very ugly grammar and code...

- value: int | boolean | identifier | \id.exp
- prefix notation
- basic arithmetic computation: `(+ | - | * | / exp exp)`
- basic comparison: `(< | == | != exp exp)`
- `(if exp exp exp) | (let id exp exp)`
- **recursion!**:  `(letrec id exp exp)`
