# CoroGLL
Experiments in GLL parsing with C++ coroutines

Abusing coroutines and blatant undefined behaviour for parsing an ambiguous and non-deterministic grammar.

## Examples
```
a < b < c
LessThanExpression
  LessThanExpression
    WordExpression
      a
    WordExpression
      b
  WordExpression
    c
```
```
a < b < c >
LessThanExpression
  WordExpression
    a
  SpecializationExpression
    WordExpression
      b
    ArgumentList
      Argument
        WordExpression
          c
```
```
a < b < c > >
SpecializationExpression
  WordExpression
    a
  ArgumentList
    Argument
      SpecializationExpression
        WordExpression
          b
        ArgumentList
          Argument
            WordExpression
              c
```
