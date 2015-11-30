
//
// This ROOT macro orginally created as a skelton using the
// "mkrootmacro" script. (davidl@jlab.org)
//

#include "StandardLabels.C"

//-------------------------
// GetRate
//-------------------------
double GetRate(int nthreads, int color, double &sigma)
{
	char fname[256];
	sprintf(fname, "rate%d.root", nthreads);
	cout << fname << endl;

	new TFile(fname);
	TTree *t = (TTree*)gROOT->FindObject("rate_tree");

	TH1D *h = new TH1D("h", "", 500, 0.0, 15.0);
	t->Project("h", "tot_rate/1000", "Entry$>300");
	h->Fit("gaus");
	TF1 *f = (TF1*)h->GetFunction("gaus");
	double mean = 0.0;
	sigma=0.0;
	if(f){
		mean = f->GetParameter(1);
		sigma = f->GetParameter(2);
	}

	return mean; // return rate in kHz. (sigma is returned by reference)
}

//-------------------------
// rate_vs_nthreads
//-------------------------
void rate_vs_nthreads(void)
{
	TColor::CreateColorWheel();
	
	

	TCanvas *c1 = new TCanvas("c1");
	c1->SetTicks();
	c1->SetGrid();

	int colors[]={kBlack, kRed,kCyan,kMagenta, kOrange};
	int Ncolors = 5;

	double threads[]={1,2,4,8,12,16,20,24,28,32,36};
	int Nthreads = 11;
	double rates[100];
	double sigmas[100];
	for(int i=0; i<Nthreads; i++){
		rates[i] = GetRate((int)threads[i], colors[i%Ncolors], sigmas[i]);
		
		cout << "nthreads="<<threads[i]<< "  rate="<<rates[i]<< " +/- " << sigmas[i]<<endl;
	}
	
	TH2D *axes = new TH2D("axes", "Rate vs. Nthreads", 100, 0.0, 36.0, 100, 0.0, 4.0);
	axes->SetStats(0);
	axes->SetXTitle("Nthreads");
	axes->SetYTitle("Rate (kHz)");

	TGraphErrors *g = new TGraphErrors(Nthreads, threads, rates, NULL, sigmas);
	g->SetMarkerColor(kRed);
	g->SetLineColor(kRed);
	g->SetLineWidth(2);
	g->SetMarkerStyle(20);
	
	// Formula below is for a curve that describes the event
	// processing rate vs. nthreads where some portion of
	// the time is spent in serial and some in parallel.
	// These are both free parameters in the fit. A third
	// parameter is the fraction of a full core that a
	// hyperthread contributes. This is included in the following
	// way:
	//
	//                1
	//  R = ----------------------
	//       Tserial  +  Tpara/N
	//
	// where the "1/N" in the denominator accounts for the
	// parallelism. The TF1 uses this to also include the
	// hyperthreads. For the first 16 threads, N = Nthreads.
	// After that, each thread contributes only a fraction
	// of a full core so N = 16 + alpha*(Nthreads-16). Here
	// alpha is the hyperthread equivalent. So, if each hyper-
	// thread were 20% of a full core, then for 17 threads,
	// N=16.2 .
	//
	// To do this in a way that TF1 can easily digest for all
	// values of N, we write:
	//
	//     N = x - (1-alpha)*(x-16)(x>16)
	//
	//  where the long second term effectively subtracts the
	// portion of a real core that the hyperthreads can't
	// provide (hence the "1-alpha"). The (x-16)(x>16) part
	// ensures that the second term only applies to the threads
	// beyond the first 16. This is what provides the kink
	// in the curve at 16.
	
	
	TF1 *rf = new TF1("rf", "1.0/([0]+[1]/(x-(1-[2])*(x-16)*(x>16)))", 1.0, 32.0);
	rf->SetParName(0, "T_serial");
	rf->SetParName(1, "T_parallel");
	rf->SetParName(2, "ht_equivalent");
	rf->SetParameter(0, 1.0);
	rf->SetParameter(1, 1.0/rates[0]);
	rf->SetParameter(2, 0.0);
	g->Fit(rf, "", "", 0.0, 32.0);
	
	double Tserial= rf->GetParameter(0); // in ms since rate is in kHz
	double Tpara= rf->GetParameter(1);   // in ms since rate is in kHz
	double ht_equiv =  rf->GetParameter(2);
	
	axes->Draw();
	g->Draw("PE1same");
	rf->Draw("same");
	
	TLatex *latex = new TLatex();
	latex->SetTextSize(0.04);
	latex->SetTextAlign(22);

	double y_scale = 4.0/10.0; // scale factor (numerator should be y-axis max)

	char str[256];
	sprintf(str, "T_{serial} = %3.1f#mus", Tserial*1000);
	latex->DrawLatex(22.0, 4.5*y_scale, str);
	sprintf(str, "T_{parallel} = %3.1f#mus", Tpara*1000);
	latex->DrawLatex(22.0, 3.5*y_scale, str);

	sprintf(str, "Hyperthread = %3.0f%% of full core", ht_equiv*100.0);
	latex->DrawLatex(22.0, 1.5*y_scale, str);
	
	double mt_eff = 1.0/(16.0*(1.0+ht_equiv)*Tserial/Tpara + 1);
	sprintf(str, "Multi-threading efficiency: %3.1f%%", 100.0*mt_eff);
	latex->DrawLatex(10.0, 9.5*y_scale, str);	
	
	double max_rate = 1.0/Tserial;
	TLine *lin =new TLine(0.0, max_rate, 36.0, max_rate);
	lin->SetLineColor(kMagenta);
	lin->SetLineStyle(2);
	lin->SetLineWidth(3);
	lin->Draw();
	
	TF1 *rfmax = (TF1*)rf->Clone("rfmax");
	rfmax->SetParameter(0, 0.0);
	rfmax->SetParameter(1, Tserial+Tpara);
	rfmax->SetParameter(2, ht_equiv);
	rfmax->SetLineColor(kBlack);
	rfmax->SetLineStyle(2);
	rfmax->Draw("same");

	StandardLabels(axes,"gluon106", "", "100k events from hd_rawdata_002931_002.evio", "DBCALCluster");

	c1->SaveAs("rate_vs_nthreads.png");
	c1->SaveAs("rate_vs_nthreads.pdf");
}

