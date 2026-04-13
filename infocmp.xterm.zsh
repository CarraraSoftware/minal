#!/usr/bin/zsh
export TERM="xterm-256color"
cmds=(am bce ccc km mc5i mir msgr npc xenl colors 
    cols it lines pairs acsc bel blink bold 
    cbt civis clear cnorm cr csr cub cub1 cud cud1 cuf cuf1 
    cup cuu cuu1 cvvis dch dch1 dim dl dl1 ech ed el el1 flash 
    home hpa ht hts ich il il1 ind indn initc invis is2 kDC kEND 
    kHOM kIC kLFT kNXT kPRV kRIT ka1 ka3 kb2 kbeg kbs kc1 kc3 kcbt
    kcub1 kcud1 kcuf1 kcuu1 kdch1 kend kent kf1 kf10 kf11 kf12 kf13
    kf14 kf15 kf16 kf17 kf18 kf19 kf2 kf20 kf21 kf22 kf23 kf24 kf25
    kf26 kf27 kf28 kf29 kf3 kf30 kf31 kf32 kf33 kf34 kf35 kf36 kf37
    kf38 kf39 kf4 kf40 kf41 kf42 kf43 kf44 kf45 kf46 kf47 kf48 kf49
    kf5 kf50 kf51 kf52 kf53 kf54 kf55 kf56 kf57 kf58 kf59 kf6 kf60 kf61
     kf62 kf63 kf7 kf8 kf9 khome kich1 kind kmous knp kpp kri mc0
    mc4 mc5 meml memu mgc nel oc op rc rep rev ri rin ritm rmacs 
    rmam rmcup rmir rmkx rmm rmso rmul rs1 rs2 sc setab setaf sgr 
    sgr0 sitm smacs smam smcup smglp smglr smgrp smir smkx smm smso 
    smul tbc u6 u7 u8 u9 vpa)

for cmd in $cmds; do
    # echo $cmd;
    echo "---"
    echo "$cmd: "
    builtin echoti $cmd | xxd 
done
