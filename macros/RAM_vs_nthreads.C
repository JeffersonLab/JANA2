

void RAM_vs_nthreads(void)
{

	TCanvas *c1 = new TCanvas("c1","",800,800);
	c1->SetTicks();
	c1->SetGrid();
	
	TH2D *axes = new TH2D("axes", "RAM usage vs. Nthreads", 100, 0.0, 33.0, 100, 0.0, 3750.0);
	axes->SetYTitle("Total RAM usage (MB)");
	axes->SetXTitle("Number of processing threads");
	axes->SetStats(0);
	axes->Draw();

	TGraph *g = new TGraph("mem.dat");
	for(int i=0; i<g->GetN(); i++){
		double x,y;
		g->GetPoint(i, x, y);
		y /= 4.0*1024.0; // convert into MB (factor of for is due to bug in /usr/bin/time)
		g->SetPoint(i, x, y);
	}
	g->Print();
	
	g->SetMarkerStyle(20);
	g->SetMarkerColor(kRed);
	g->Draw("Psame");

	g->Fit("pol1", "", "same", 4.0, 32.0);
	TF1 *f = g->GetFunction("pol1");

	double ram_base = f->GetParameter(0);
	double ram_per_thread = f->GetParameter(1);
		
		
	cout << "ram_per_thread="<<ram_per_thread<<endl;
	
	c1->SaveAs("RAM_vs_nthreads.png");
	c1->SaveAs("RAM_vs_nthreads.pdf");
}

