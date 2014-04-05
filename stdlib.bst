(def {True} (bool 1))
(def {False} (bool 0))

(def {fun} (\ {f b} {
    def (head f) (\ (tail f) b)
}))

(fun {unpack f l} {
    eval (join (list f) l)
})

(fun {pack f & xs} {f xs})

(def {curry} {unpack})
(def {uncurry} {pack})

(fun {do & l} {
    if (== l {})
        {{}}
        {last l}
})

(fun {let b} {
    ((\ {_} b) ())
})

(fun {fst l} { eval (head l) })
(fun {snd l} { eval (head (tail l)) })

(fun {nth n l} {
    if (== n 0)
        {fst l}
        {nth (- n 1) (tail l)}
})
(fun {last l} {nth (- (len l) 1) l})

(fun {elem x l} {
    if (== l {})
        {(bool 0)}
        {if (== x (fst l)) {(bool 1)} {elem x (tail l)}}
})

(fun {map f l} {
    if (== l {})
        {{}}
        {join (list (f (fst l))) (map f (tail l))}
})

(fun {filter f l} {
    if (== l {})
        {{}}
        {join (if (f (fst l)) {head l} {{}}) (filter f (tail l))}
})

(fun {foldl f z l} {
    if (== l {})
        {z}
        {foldl f (f z (fst l)) (tail l)}
})

(fun {sum l} {foldl + 0 l})
(fun {product l} {foldl * 1 l})
