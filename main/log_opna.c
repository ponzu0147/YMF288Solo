void log_opna( int _dev_no, int _dev_ex, int _reg, int _dat) {

	int dev = _dev_no;
	int dev_ex = _dev_ex & 1;
	int reg = _reg;
	int dat = _dat;

	static int timerA[MAXDEVICE]         = { 0 };
	static int prescaler[MAXDEVICE]      = { 0 };
	static int SSGEG_Freq[MAXDEVICE]     = { 0 };
	static int SSGTune[MAXDEVICE][3]     = { 0 };
	static int FNum[MAXDEVICE][6]        = { 0 };
	static int Oct[MAXDEVICE][6]         = { 0 };
	static int fm3ex[MAXDEVICE]          = { 0 };
	static int ADPCM_start[MAXDEVICE]    = { 0 };
	static int ADPCM_stop[MAXDEVICE]     = { 0 };
	static int ADPCM_prescale[MAXDEVICE] = { 0 };
	static int ADPCM_deltan[MAXDEVICE]   = { 0 };
	static int ADPCM_limit[MAXDEVICE]    = { 0 };
	static int dacmode[MAXDEVICE]        = { 0 };
	const int exop[4] = { 0, 3, 1, 2 };
	const int exsl[4] = { 0, 2, 1, 3 };
	const char rhythm[6] = {'B', 'S', 'C', 'H', 'T', 'R'};
	int ch = 0;
	int sl = 0;
	int j;

	switch (reg) {
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
		if (dev_ex==0) {
			// SSG
			switch (reg) {
			case 0x00:
			case 0x02:
			case 0x04:
				ch = reg>>1;
				SSGTune[dev][ch] &= 0xff00;
				SSGTune[dev][ch] |= dat;
				printf("SSG#%d   Fine Tune=$%02x($%03x)",ch, dat, SSGTune[dev][ch]);
				break;

			case 0x01:
			case 0x03:
			case 0x05:
				ch = reg>>1;
				SSGTune[dev][ch] &= 0x00ff;
				SSGTune[dev][ch] |= ((dat&0x0f)<<8);
				printf("SSG#%d Coarse Tune=$%02x($%03x)",ch, dat, SSGTune[dev][ch]);
				break;

			case 0x06:
				printf("SSG Noise Period=%2d",dat&0x1f); break;

			case 0x07:
				printf("SSG Mixer/IO  TONE:");
				for (j=0; j<3; j++) {
					if ((dat&(1<<j))==0) printf(" %c",'0'+j); else printf(" -");
				}
				printf("  NOISE:");
				for (j=0; j<3; j++) {
					if ((dat&(1<<(j+3)))==0) printf(" %c",'0'+j); else printf(" -");
				}
				break;

			case 0x08:
			case 0x09:
			case 0x0a:
				ch = reg-0x08;
				printf("SSG#%d VOL=%2d",ch, dat&0x0f);
				if (dat&0x10) printf("[HW Env ON]");
				break;

			case 0x0b:
				SSGEG_Freq[dev] &= 0xff00;
				SSGEG_Freq[dev] |= dat;
				printf("SSG Env   Fine Tune=$%02x($%04x)",dat,SSGEG_Freq[dev]); break;

			case 0x0c:
				SSGEG_Freq[dev] &= 0x00ff;
				SSGEG_Freq[dev] |= (dat<<8);
				printf("SSG Env Coarse Tune=$%02x($%04x)",dat,SSGEG_Freq[dev]); break;

			case 0x0d:
				printf("SSG Envelope Shape=%2d",dat); break;

			case 0x0e:
				printf("SSG I/O Port A=$%02x",dat); break;

			case 0x0f:
				printf("SSG I/O Port B=$%02x",dat); break;
			}
		}
		else {
			// ADPCM
			switch (reg) {

			case 0x00:
				printf("ADPCM CONTROL1 :",dat);
				if (dat&0x01) printf(" RESET");
				if (dat&0x08) printf(" SP OFF");
				if (dat&0x10) printf(" REPEAT");
				if (dat&0x20) printf(" MEM DATA");
				if (dat&0x40) printf(" REC");
				if (dat&0x80) printf(" START");
				break;

			case 0x01:
				printf("ADPCM CONTROL2 :",dat);
				if (dat&0x01) printf(" ROM"); else printf(" RAM");
				if (dat&0x02) printf(" RAM_TYPE=x8"); else printf(" RAM_TYPE=x1");
				if (dat&0x04) printf(" DA/AD");
				if (dat&0x08) printf(" SAMPLE");
				if (dat&0x40) printf(" L");
				if (dat&0x80) printf(" R");
				break;

			case 0x02:
				ADPCM_start[dev] &= 0xff00;
				ADPCM_start[dev] |= dat;
				printf("ADPCM START ADR(L)=$%02x($%04x)",dat,ADPCM_start[dev]);
				break;

			case 0x03:
				ADPCM_start[dev] &= 0x00ff;
				ADPCM_start[dev] |= (dat<<8);
				printf("ADPCM START ADR(H)=$%02x($%04x)",dat,ADPCM_start[dev]);
				break;

			case 0x04:
				ADPCM_stop[dev] &= 0xff00;
				ADPCM_stop[dev] |= dat;
				printf("ADPCM  STOP ADR(L)=$%02x($%04x)",dat,ADPCM_stop[dev]);
				break;

			case 0x05:
				ADPCM_stop[dev] &= 0x00ff;
				ADPCM_stop[dev] |= (dat<<8);
				printf("ADPCM  STOP ADR(H)=$%02x($%04x)",dat,ADPCM_stop[dev]);
				break;

			case 0x06:
				ADPCM_prescale[dev] &= 0xff00;
				ADPCM_prescale[dev] |= dat;
				printf("ADPCM PRESCALE(L)=$%02x($%04x)",dat,ADPCM_prescale[dev]);
				break;

			case 0x07:
				ADPCM_prescale[dev] &= 0x00ff;
				ADPCM_prescale[dev] |= ((dat&0x0f)<<8);
				printf("ADPCM PRESCALE(H)=$%02x($%04x)",dat,ADPCM_prescale[dev]);
				break;

			case 0x08:
				printf("ADPCM DATA=$%02x",dat);
				break;

			case 0x09:
				ADPCM_deltan[dev] &= 0x00ff;
				ADPCM_deltan[dev] |= (dat<<8);
				printf("ADPCM DELTA-N(L)=$%02x($%04x)",dat,ADPCM_deltan[dev]);
				break;

			case 0x0a:
				ADPCM_deltan[dev] &= 0x00ff;
				ADPCM_deltan[dev] |= (dat<<8);
				printf("ADPCM DELTA-N(H)=$%02x($%04x)",dat,ADPCM_deltan[dev]);
				break;

			case 0x0b:
				printf("ADPCM EG-CONTROL=$%02x",dat);
				break;

			case 0x0c:
				ADPCM_limit[dev] &= 0xff00;
				ADPCM_limit[dev] |= dat;
				printf("ADPCM LIMIT ADR(L)=$%02x($%04x)",dat,ADPCM_limit[dev]);
				break;

			case 0x0d:
				ADPCM_limit[dev] &= 0x00ff;
				ADPCM_limit[dev] |= (dat<<8);
				printf("ADPCM LIMIT ADR(H)=$%02x($%04x)",dat,ADPCM_limit[dev]);
				break;

			case 0x0e:
				printf("ADPCM DAC DATA=$%02x",dat);
				break;

			case 0x0f:
				printf("PCM DATA=$%02x",dat);
				break;
			}
		}
		break;

	case 0x10:
		if (dev_ex==0) {
			// RHYTHM
			printf("RHY Key-",dat);
			if (dat&0x80) printf("OFF :"); else printf("ON :");
			for (j=0; j<6; j++) {
				if (dat&(1<<j)) printf(" %c",rhythm[j]); else printf(" -");
			}
		}
		else {
			// ADPCM
			printf("ADPCM FLAG CONTROL :");
			if (dat&0x01) printf(" MSK_TA");
			if (dat&0x02) printf(" MSK_TB");
			if (dat&0x04) printf(" MSK_EOS");
			if (dat&0x08) printf(" MSK_BRDY");
			if (dat&0x10) printf(" MSK_ZERO");
			if (dat&0x80) printf(" IRQ_RST");
			break;
		}
		break;
	// RHYTHM
	case 0x11:
		if (dev_ex) break;
		printf("RHY Total Level=%03d",dat); break;

	case 0x12:
		if (dev_ex) break;
		printf("RHY Test : %02x",dat); break;

	case 0x18:
		if (dev_ex) break;
		printf("RHY / Bass drum  ");
		goto rhythm_vol;
	case 0x19:
		if (dev_ex) break;
		printf("RHY / Snare drum ");
		goto rhythm_vol;
	case 0x1a:
		if (dev_ex) break;
		printf("RHY / Cymbal     ");
		goto rhythm_vol;
	case 0x1b:
		if (dev_ex) break;
		printf("RHY / Hi-Hat     ");
		goto rhythm_vol;
	case 0x1c:
		if (dev_ex) break;
		printf("RHY / Tom        ");
		goto rhythm_vol;
	case 0x1d:
		if (dev_ex) break;
		printf("RHY / Rim shot   ");
	rhythm_vol:
		printf("VOL=%2d", dat&0x3f);
		if (dat&0x80) printf(" L");
		if (dat&0x40) printf(" R");
		break;

	// FM
	case 0x21:
		if (dev_ex) break;
		printf("FM Test : %02x",dat); break;

	case 0x22:
		if (dev_ex) break;
		printf("FM HW LFO ");
		if (dat&0x40) printf("ON "); else printf("OFF ");
		printf("Freq: %0d",dat&7);
		break;

	case 0x24:
		if (dev_ex) break;
		timerA[dev] &= 0x003;
		timerA[dev] |= dat<<2;
		printf("TIMER-A1=$%02x($%03x)",dat, timerA[dev]); break;

	case 0x25:
		if (dev_ex) break;
		timerA[dev] &= 0x3fc;
		timerA[dev] |= dat&0x03;
		printf("TIMER-A2=$%02x($%03x)",dat, timerA[dev]); break;

	case 0x26:
		if (dev_ex) break;
		printf("TIMER-B=%3d",dat); break;

	case 0x27:
		if (dev_ex) break;
		printf("FM3Mode:");
		if ((dat&0xc0)==0x80) {
			printf("CSM");
			fm3ex[dev] = 1;
		}
		else if ((dat&0xc0)==0x40) {
			printf("EFFECT");
			fm3ex[dev] = 1;
		}
		else if ((dat&0x00)==0x00) {
			printf("NORMAL");
			fm3ex[dev] = 0;
		}
		else {
			printf("(undefined)");
		}
		printf("  TIMER-A:");
		if (dat&0x01) printf(" START");  else printf(" STOP ");
		if (dat&0x04) printf(" ENABLE "); else printf(" DISABLE");
		if (dat&0x10) printf(" RESET");  else printf(" -----");
		printf("  TIMER-B:");
		if (dat&0x02) printf(" START");  else printf(" STOP ");
		if (dat&0x08) printf(" ENABLE "); else printf(" DISABLE");
		if (dat&0x20) printf(" RESET");  else printf(" -----");
		break;

	case 0x28:
		if (dev_ex) break;
		ch = dat & 0x07;
		if (ch>3) ch--;
		printf("FM#%d Key-ON/OFF slot:",ch);
		for (j=0; j<4; j++) {
			if (dat&(1<<(j+4))) printf(" %d",j+1); else printf(" -");
		}
		break;

	case 0x29:
		if (dev_ex) break;
		printf("IRQ Enable:",dat);
		if (dat&0x01) printf(" ENABLE_TIMERA");
		if (dat&0x02) printf(" ENABLE_TIMERB");
		if (dat&0x04) printf(" ENABLE_EOS");
		if (dat&0x08) printf(" EN_BRDY");
		if (dat&0x10) printf(" EN_ZERO");
		if (dat&0x80) printf(" ENABLE_FM4-6");
		break;

	case 0x2a:
		// OPN2 only
		if (dacmode[dev]) printf("FM#5 DAC $%02x",dat);
		break;

	case 0x2b:
		// OPN2 only
		dacmode[dev] = dat>>7;
		if (dacmode[dev]) printf("FM#5 DAC ENABLE"); else printf("FM#5 DAC DISABLE");
		break;

	case 0x2d:
		if (dev_ex) break;
		if (prescaler[dev] == 0x2e) {
			printf("Prescaler FM=1/3 SSG=1/2");
		}
		else {
			printf("Prescaler FM=1/6 SSG=1/4");
		}
		prescaler[dev] = 0x2d;
		break;

	case 0x2e:
		if (dev_ex) break;
		if (prescaler[dev] == 0x2d) {
			printf("Prescaler FM=1/3 SSG=1/2");
		}
		else {
			printf("Prescaler");
		}
		prescaler[dev] = 0x2e;
		break;

	case 0x2f:
		if (dev_ex) break;
		printf("Prescaler FM=1/2 SSG=1/1");
		prescaler[dev] = 0x2f;
		break;

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3c:
	case 0x3d:
	case 0x3e:
		// DT/ML
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x30)>>2];
		printf("FM#%d (slot%d) DT=%d  ML=%2d",ch, sl+1, (dat&0x30)>>4, dat&0x0f);
		break;

	case 0x40:
	case 0x41:
	case 0x42:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x48:
	case 0x49:
	case 0x4a:
	case 0x4c:
	case 0x4d:
	case 0x4e:
		// TL
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x40)>>2];
		printf("FM#%d (slot%d) TL=%3d",ch, sl+1, dat&0x7f);
		break;

	case 0x50:
	case 0x51:
	case 0x52:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x58:
	case 0x59:
	case 0x5a:
	case 0x5c:
	case 0x5d:
	case 0x5e:
		// KS/AR
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x50)>>2];
		printf("FM#%d (slot%d) KS=%d  AR=%2d",ch, sl+1, dat>>6, dat&0x1f);
		break;

	case 0x60:
	case 0x61:
	case 0x62:
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x68:
	case 0x69:
	case 0x6a:
	case 0x6c:
	case 0x6d:
	case 0x6e:
		// AMON/DR
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x60)>>2];
		printf("FM#%d (slot%d) AMON=%d  DR=%2d",ch, sl+1, dat>>7, dat&0x1f);
		break;

	case 0x70:
	case 0x71:
	case 0x72:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x78:
	case 0x79:
	case 0x7a:
	case 0x7c:
	case 0x7d:
	case 0x7e:
		// SR
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x70)>>2];
		printf("FM#%d (slot%d) SR=%2d",ch, sl+1, dat&0x1f);
		break;

	case 0x80:
	case 0x81:
	case 0x82:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x88:
	case 0x89:
	case 0x8a:
	case 0x8c:
	case 0x8d:
	case 0x8e:
		// SL/RR
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x80)>>2];
		printf("FM#%d (slot%d) SL=%2d  RR=%2d",ch, sl+1, dat>>4, dat&0x0f);
		break;

	case 0x90:
	case 0x91:
	case 0x92:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x98:
	case 0x99:
	case 0x9a:
	case 0x9c:
	case 0x9d:
	case 0x9e:
		// SSG-EG
		ch = (reg&0x03) + 3*dev_ex;
		sl = exsl[(reg-0x90)>>2];
		printf("FM#%d (slot%d) SSG-EG=%2d",ch, sl+1, dat&0x0f);
		break;

	case 0xa0:
	case 0xa1:
	case 0xa2:
		// F-Num.1
		ch = (reg&0x03) + 3*dev_ex;
		FNum[dev][ch] &= 0x700;
		FNum[dev][ch] |= dat;
		if (((!fm3ex[dev]) && (reg!=0xa2)) || dev_ex) {
			// 通常モード
			printf("FM#%d F-NUM1=$%02x(OCT=%d $%03x)",ch, dat, Oct[dev][ch], FNum[dev][ch]);
		}
		else {
			// 効果音モード
			printf("FM#2 (slot4) F-NUM1=$%02x (OCT=%d $%03x)",dat, Oct[dev][ch], FNum[dev][ch]);
		}
		break;

	case 0xa8:
	case 0xa9:
	case 0xaa:
		// F-Num.1 (CH3) 効果音モード専用
		if (dev_ex) break;
		sl = exop[(reg&0x03)+1];
		FNum[dev][2+sl] &= 0x700;
		FNum[dev][2+sl] |= dat;
		printf("FM#2 (slot%d) F-NUM1=$%02x (OCT=%d $%03x)",sl, dat, Oct[dev][2+sl], FNum[dev][2+sl]);
		break;

	case 0xa4:
	case 0xa5:
	case 0xa6:
		// BLOCK/F-Num.2
		ch = (reg&0x03) + 3*dev_ex;
		FNum[dev][ch] &= 0x0ff;
		FNum[dev][ch] |= (dat&0x07)<<8;
		Oct[dev][ch] = (dat&0x38)>>3;
		if (((!fm3ex[dev]) && (reg!=0xa6)) || dev_ex) {
			// 通常モード
			printf("FM#%d BLOCK=%d F-NUM2=$%02x",ch, Oct[dev][ch]+1, dat&0x07);
			break;
		}
		else {
			// 効果音モード
			printf("FM#2 (slot4) BLOCK=%d F-NUM2=$%02x",Oct[dev][ch]+1, dat&0x07);
		}
		break;

	case 0xac:
	case 0xad:
	case 0xae:
		// Block / F-Num.2 (CH3) 効果音モード専用
		if (dev_ex) break;
		sl = exop[(reg&0x03)+1];
		FNum[dev][2+sl] &= 0x0ff;
		FNum[dev][2+sl] |= (dat&0x07)<<8;
		Oct[dev][2+sl] = (dat&0x38)>>3;
		printf("FM#2 (slot%d) BLOCK=%d F-NUM2=$%02x",sl, Oct[dev][2+sl]+1, dat&0x07);
		break;

	case 0xb0:
	case 0xb1:
	case 0xb2:
		// FB/CONNECT
		ch = (reg&0x03) + 3*dev_ex;
		printf("FM#%d FB=%d AL=%d",ch, (dat&0x38)>>3, dat&0x07);
		break;

	case 0xb4:
	case 0xb5:
	case 0xb6:
		// PAN/AMS/PMS
		ch = (reg&0x03) + 3*dev_ex;
		printf("FM#%d AMS=%d PMS=%d",ch, (dat&0x30)>>4, dat&0x07);
		if (dat&0x80) printf(" L");
		if (dat&0x40) printf(" R");
		break;
	}
	return;
}
