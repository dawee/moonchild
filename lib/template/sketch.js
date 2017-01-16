module.exports = () => `
/*
 *
 *
 *                             :           :
 *                            t#,         t#,     L.
 *                           ;##W.       ;##W.    EW:        ,ft
 *             ..       :   :#L:WE      :#L:WE    E##;       t#E
 *            ,W,     .Et  .KG  ,#D    .KG  ,#D   E###t      t#E
 *           t##,    ,W#t  EE    ;#f   EE    ;#f  E#fE#f     t#E
 *          L###,   j###t f#.     t#i f#.     t#i E#t D#G    t#E
 *        .E#j##,  G#fE#t :#G     GK  :#G     GK  E#t  f#E.  t#E
 *       ;WW; ##,:K#i E#t  ;#L   LW.   ;#L   LW.  E#t   t#K: t#E
 *      j#E.  ##f#W,  E#t   t#f f#:     t#f f#:   E#t    ;#W,t#E
 *    .D#L    ###K:   E#t    f#D#;       f#D#;    E#t     :K#D#E
 *   :K#t     ##D.    E#t     G#t         G#t     E#t      .E##E
 *   ...      #G      ..       t           t      ..         G#E
 *
 *
 *                                                                    ED.
 *                                  .,                                E#Wi
 *                                 ,Wt .    .      t              i   E###G.
 *                                i#D. Di   Dt     Ej            LE   E#fD#W;
 *                               f#f   E#i  E#i    E#,          L#E   E#t t##L
 *                             .D#i    E#t  E#t    E#t         G#W.   E#t  .E#K,
 *                            :KW,     E#t  E#t    E#t        D#K.    E#t    j##f
 *                            t#f      E########f. E#t       E#K.     E#t    :E#K:
 *    Lua 5.3.3                ;#G     E#j..K#j... E#t     .E#E.      E#t   t##L
 *                              :KE.   E#t  E#t    E#t    .K#E        E#t .D#W;
 *                               .DW:  E#t  E#t    E#t   .K#D         E#tiW#G.
 *    for Arduino boards           L#, f#t  f#t    E#t  .W#G          E#K##i
 *                                  jt  ii   ii    E#t :W##########Wt E##D.
 *                                                 ,;. :,,,,,,,,,,,,,.E#t
 *                                                                    L:
 */

#include "moonchild.h"

void setup() {
  moon_init();
  moon_run_generated();
}

void loop() {
  moon_arch_update();
}
`;
