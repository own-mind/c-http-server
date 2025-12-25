#ifndef __CHAR_STREAM_H
#define __CHAR_STREAM_H

struct CharStream;
typedef struct CharStream CharStream;

char peek(CharStream *stream);
char next(CharStream *stream);
void skip(CharStream *stream);
int eof(CharStream *stream);
void freeStream(CharStream *stream);

CharStream *createStringStream(char *data);
CharStream *createSocketStream(int clientSockfd);

#endif // !CHAR_STREAM
