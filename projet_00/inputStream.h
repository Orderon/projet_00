#pragma once
class inputStream
{
public:
	inputStream();
	int openStream(void);

	void inputStream::setCallback();
	void inputStream::startStream();
	~inputStream();


private:
	IDeckLinkIterator*		deckLinkIterator;
	IDeckLink*				deckLink;
	IDeckLinkInput*			deckLinkInput;
};

