//
// EAE type 182 (for PDP8/E!)
//
int group3(int xmd)
{
	int qtm, xtm = xmd & 07721, xtc, earg, farg, dphi;
	int tm1, tm2;

	if (xmd == 07431)					// Switch modes
	{
		eaemd = 1; return 0;
	}
	if (xmd == 07447 || xmd == 07777)
	{
		gtf = eaemd = 0; return 0;
	}

	if ((xmd == 07573 || xmd == 07575) && eaemd) {      // DPIC and DCM here
		if (xmd & 4) {
			MQ = ~MQ;                            // Complement
			ACC = ~ACC;
			MQ &= 07777;
		}
		ACC &= 07777;
		MQ = MQ + 1;
		if (MQ & 010000)
			ACC = ACC + 1;
		MQ &= 07777;
		ACC &= 017777;
		return 0;
	}
	if (xmd & 0200)
		ACC = ACC & 010000;
	if ((xmd & 0120) == 0120) {
		qtm = ACC;
		ACC = MQ | (ACC & 010000);
		MQ = qtm & 07777;
	}
	else {
		if (xmd & 020) {
			MQ = ACC & 07777;
			ACC &= 010000;
		}
		if (xmd & 0100)
			ACC |= MQ;
	}
	if (xmd & 040 && !eaemd)
		ACC |= EAESC;
	if (eaemd == 0) gtf = 0;
	if ((PC & 07770) == 010)
		mem[PC + ifl]++;
	earg = mem[PC + ifl] & 07777;
	//				if ( eaemd && ( earg&07770 )==010 ) 
	//                      mem[earg]++;

	xtc = eaemd ? xmd & 056 : xmd & 016;
	switch (xtc) {
	case 0:
		return 0;
		break;
	case 02:
		if (eaemd)
		{
			EAESC = ACC & 037;
			ACC &= 010000;
		}
		else {
			EAESC = (~earg) & 037;
			return 1;
		}
		return 0;
		break;
	case 04:                                        // MUY
		if (eaemd)
			earg = mem[earg + dfl];
		MQ = MQ * earg;
		xtm = ((MQ) >> 12) & 07777;
		MQ = ((ACC & 07777) + MQ) & 07777;
		ACC = xtm;
		EAESC = 0;
		return 1;
		break;
	case 06:                                        // DVI
		if (eaemd)
			earg = mem[earg + dfl];
		if ((ACC & 07777) >= earg) {               /* overflow? */
			ACC = ACC | 010000;                     /* set link */
			MQ = ((MQ << 1) + 1) & 07777;           /* rotate MQ */
			EAESC = 0;                                 /* no shifts */
		}
		else {
			xtm = ((ACC & 07777) << 12) | MQ;
			MQ = xtm / earg;
			ACC = xtm % earg;
			EAESC = 015;                               /* 13 shifts */
		}
		return 1;
		break;
	case 010:                                          // NMI
		xtm = (ACC << 12) | MQ;                    /* preserve link */
		for (EAESC = 0; ((xtm & 017777777) != 0) &&
			(xtm & 040000000) == ((xtm << 1) & 040000000); EAESC++)
			xtm = xtm << 1;
		ACC = (xtm >> 12) & 017777;
		MQ = xtm & 07777;
		if (eaemd && ((ACC & 07777) == 04000) && (MQ == 0))
			ACC = ACC & 010000;                     /* clr if 4000'0000 */
		return 0;
		break;
	case 012:                                          // SHL
		EAESC = eaemd ? 037 : 0;
		for (xtm = eaemd ? 0 : -1; xtm < (earg & 037); xtm++)
		{
			MQ = MQ << 1;
			ACC = ACC << 1;
			if (MQ & 010000)
				ACC += 1;
		}
		MQ &= 07777;
		ACC &= 017777;

		return 1;
		break;
	case 014:
		EAESC = eaemd ? 037 : 0;
		//printf("ACC:%05o MQ:%04o EARG:%04o\r\n",ACC,MQ,earg);
		if (ACC & 04000)
			ACC |= 010000;
		else
			ACC &= 07777;
		for (xtm = eaemd ? 0 : -1; xtm < (earg & 037); xtm++)
		{
			gtf = MQ & 1;
			MQ = MQ >> 1;
			if (ACC & 04000)
				ACC |= 030000;
			if (ACC & 01)
				MQ |= 04000;
			ACC = ACC >> 1;
		}
		MQ &= 07777;
		ACC &= 017777;
		return 1;
		break;
	case 016:
		EAESC = eaemd ? 037 : 0;
		ACC &= 07777;
		for (xtm = eaemd ? 0 : -1; xtm < (earg & 037); xtm++)
		{
			gtf = MQ & 1;
			MQ = MQ >> 1;
			if (ACC & 01)
				MQ |= 04000;
			ACC = ACC >> 1;
		}
		MQ &= 07777;
		ACC &= 07777;
		return 1;
		break;
	case 040:				// SCA
		if (eaemd) {
			ACC &= 010000;
			ACC |= EAESC;
		}
		return 0;
	case 042:
		dphi = mem[earg + dfl + 1];
		earg = mem[earg + dfl];
		ACC &= 07777;
		MQ = MQ + earg;
		if (MQ & 010000)
			ACC = ACC + 1;
		ACC = (ACC + dphi) & 017777;
		MQ &= 07777;
		return 1;
	case 044:
		mem[dfl + earg++] = MQ;
		mem[dfl + earg] = ACC & 07777;
		return 1;
	case 050:				// DPSZ
		if ((ACC & 07777) + MQ == 0)
			return 1;
		break;
	case 056:               // SAM
		ACC &= 07777;
		tm1 = (ACC & 04000) ? ACC - 010000 : ACC;
		tm2 = (MQ & 04000) ? MQ - 010000 : MQ;
		gtf = (tm1 <= tm2) ? 1 : 0;
		ACC = MQ - ACC;
		ACC &= 07777;
		ACC |= (ACC <= MQ) ? 010000 : 0;
	default:
		return 0;
	}
	return 0;
}
