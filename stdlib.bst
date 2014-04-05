;;;
;;; Bugsp Standard lib
;;;

;;; Atoms
(def {True} (bool 1))
(def {False} (bool 0))

;;; Functions

; define a new function
(def {fun} (\ {f b} {
    def (head f) (\ (tail f) b)
}))

; open new scope
(fun {let b} {
    ((\ {_} b) ())
})

; unpack List to function
(fun {unpack f l} {
    eval (join (list f) l)
})

; unapply list to function
(fun {pack f & xs} {f xs})

; aliases
(def {curry} {unpack})
(def {uncurry} {pack})

; perform in sequence
(fun {do & l} {
    if (== l {})
        {{}}
        {last l}
})

;;; Numeric functions

; min of args
(fun {min & xs} {
    if (== (tail xs) {}) {fst xs}
        {do
            (= {rest} (unpack min (tail xs)))
            (= {item} (fst xs))
            (if (< item rest) {item} {rest})
        }
})

; max of args
(fun {max & xs} {
    if (== (tail xs) {}) {fst xs}
        {do
            (= {rest} (unpack min (tail xs)))
            (= {item} (fst xs))
            (if (> item rest) {item} {rest})
        }
})

;;; conditionals

(fun {select & cs} {
    if (== cs {})
        {error "No selection found"}
        {if (fst (fst cs)) {snd (fst cs)} {unpack select (tail cs)}}
})

(fun {case x & xs} {
    if (== cs {})
        {error "No case found"}
        {if (== x (fst (fst cs)))
            {snd (fst cs)}
            {unpack case (join (list x) (tail cs))}
        }
})

;;; List

; first, second item in list
(fun {fst l} { eval (head l) })
(fun {snd l} { eval (head (tail l)) })

; nth item
(fun {nth n l} {
    if (== n 0)
        {fst l}
        {nth (- n 1) (tail l)}
})

; last item
(fun {last l} {nth (- (len l) 1) l})

; length of list
(fun {len l} {
    if (== l {})
        {0}
        {(+ 1 (len (tail l)))}
})

; apply function to list
(fun {map f l} {
    if (== l {})
        {{}}
        {join (list (f (fst l))) (map f (tail l))}
})

; apply filter to list
(fun {filter f l} {
    if (== l {})
        {{}}
        {join (if (f (fst l)) {head l} {{}}) (filter f (tail l))}
})

; reverse a list
(fun {reverse l} {
    if (== l {})
        {{}}
        {join (reverse (tail l)) (head l)}
})

; fold left
(fun {foldl f z l} {
    if (== l {})
        {z}
        {foldl f (f z (fst l)) (tail l)}
})

; fold right
(fun {foldr r z l} {
    if (== l {})
        {z}
        {f (fst l) (foldr f z (tail l))}
})

; sum and product
(fun {sum l} {foldl + 0 l})
(fun {product l} {foldl * 1 l})

; take n items
(fun {take n l} {
    if (== n 0)
        {{}}
        {join (head l) (take (- n 1) (tail l))}
})

; drop n items
(fun {drop n l} {
    if (== n 0)
        {l}
        {drop (- n 1) (tail l)}
})

; takewhile
(fun {takewhile f l} {
    if (not (unpack f (head l)))
        {{}}
        {join (head l) (takewhile f (tail l))}
})

; dropwhile
(fun {dropwhile f l} {
    if (not (unpack f (head l)))
        {l}
        {dropwhile f (tail l)}
})

; elem in list
(fun {elem x l} {
    if (== l {})
        {(bool 0)}
        {if (== x (fst l)) {True} {elem x (tail l)}}
})

; zip
(fun {zip x y} {
    if (|| (== x {}) (== y {}))
        {{}}
        {join (list (join (head x) (head y))) (zip (tail x) (tail y))}
})

; unzip
(fun {unzip l} {
    if (== l {})
        {{{} {}}}
        {do
            (= {x} (fst l))
            (= {xs} (unzip (tail l)))
            (list (join (head x) (fst xs)) (join (tail x) (snd xs)))
        }
})
