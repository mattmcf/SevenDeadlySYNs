DesignDoc.pdf: DesignDoc.dvi
	dvipdfm ./DesignDoc.dvi
	
DesignDoc.dvi: Client.tex DataStructures.tex DesignDoc.tex Network.tex preamble.tex Tracker.tex ulem.sty
	latex ./DesignDoc.tex
	
clean:
	rm ./DesignDoc.dvi
	rm ./DesignDoc.pdf
	