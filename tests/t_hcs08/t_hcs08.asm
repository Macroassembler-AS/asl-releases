        cpu     68HCS08
        page    0

        bgnd

        cphx    #$55aa
        cphx    $20
        cphx    $2030
        cphx    $40,sp

        ldhx    #$55aa
        ldhx    $20
        ldhx    $2030
        ldhx    ,x
        ldhx    $40,x
        ldhx    $4050,x
        ldhx    $40,sp

        sthx    $20
        sthx    $2030
        sthx    $40,sp
