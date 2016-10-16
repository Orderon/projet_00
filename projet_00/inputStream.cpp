#include "stdafx.h"
#include "inputStream.h"
#include <list>



inputStream::inputStream()
{
	deckLinkIterator = NULL;
	deckLink = NULL;
	deckLinkInput = NULL;


	HRESULT	result;
	printf("start of init\n");
	// Initialize COM on this thread
	result = CoInitialize(NULL);
	if (FAILED(result))
	{
		fprintf(stderr, "Initialization of COM failed - result = %08x.\n", result);
		//	return 1;
	}
	else {
		printf("COM initialized\n");
	}

}

int inputStream::openStream() {

	HRESULT	result;
	unsigned int framecount = 0;
	//IDeckLinkDisplayModeIterator* displayIterator = NULL;
	_BMDDisplayModeSupport* displayModeSupport=NULL;
	IDeckLinkDisplayMode* displayMode = NULL;
	std::list<IDeckLink*> deckLinkList;




	result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&deckLinkIterator);
	if (FAILED(result)) {
		printf("Creation of DeckLinkIterator failed. \n");
		return EXIT_FAILURE;
	}
	printf("DeckLinkIterator created \n");
	




	deckLinkIterator->Next(&deckLink);
	if(deckLink==NULL){
		printf("ERROR: no DeckLink device in list!\n");
		return EXIT_FAILURE;
	}
	
	result = deckLink->QueryInterface(IID_IDeckLinkInput, (void **)&deckLinkInput);
	if (FAILED(result)) {
		printf("Creation of QueryInterface failed!\n");
		return EXIT_FAILURE;
	}
	deckLinkInput->GetAvailableVideoFrameCount(&framecount);
	std::cout << "framcount available: " << framecount << "\n";
	//deckLinkInput->GetDisplayModeIterator(&displayIterator);

	_BMDDisplayMode setDisplayMode = _BMDDisplayMode::bmdModeHD720p50;
	_BMDPixelFormat setPixelFormat = _BMDPixelFormat::bmdFormat8BitYUV;
	_BMDVideoInputFlags setInputFlag = _BMDVideoInputFlags::bmdVideoInputFlagDefault;
	//deckLinkInput->DoesSupportVideoMode(setDisplayMode, setPixelFormat, setInputFlag, displayModeSupport, &displayMode);

	try{
	deckLinkInput->EnableVideoInput(setDisplayMode, setPixelFormat, setInputFlag);
	printf("whatever\n");
	}
	catch (...) {
		std::cout << "enable video input failed: \n\n";
	}

	return EXIT_SUCCESS;
}

void inputStream::setCallback() {
	IDeckLinkInputCallback* callback;
}
void inputStream::startStream() {

}
inputStream::~inputStream()
{
	if (deckLinkInput)
		deckLinkInput->StopStreams();
	CoUninitialize();
}
