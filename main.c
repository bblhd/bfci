#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <time.h>

void tty_raw_mode(void) {
	struct termios tty_attr;
	
	tcgetattr(0,&tty_attr);
	
	/* Set raw mode. */
	tty_attr.c_lflag &= (~(ICANON|ECHO));
	tty_attr.c_cc[VTIME] = 0;
	tty_attr.c_cc[VMIN] = 1;
	
	tcsetattr(0,TCSANOW,&tty_attr);
}

struct Instruction {
	enum {NO, YES} end, out, in;
	int math;
	int seek;
	enum {ZERO, NONZERO} cond;
	int jump;
};

int findMatchingBracketDist(struct Instruction *instr) {
	int depth = 0;
	int dist = 0;
	do {
		if ((instr-dist)->jump < 0) depth++;
		if ((instr-dist)->jump > 0) depth--;
		dist++;
	} while (depth > 0);
	dist--;
	return dist;
}

int charmatch(char *match, char c) {
	while (*match != '\0') if (*match++ == c) return 1;
	return 0;
}

int storeInstruction(char c, struct Instruction *instr) {
	if (c=='>' || c=='<' || c=='+' || c=='-' || c==',' || c=='.' || c=='[' || c==']') {
		int next = 0;
		if (
			instr->jump != 0
			|| (charmatch(",.+-", c) && instr->seek != 0)
			|| (charmatch(",.", c) && instr->math != 0)
			|| (c=='.' && instr->in != 0)
		) next=1;
		if (next) instr++;
		
		if (c=='>') instr->seek++;
		if (c=='<') instr->seek--;
		if (c=='+') instr->math++;
		if (c=='-') instr->math--;
		if (c==',') instr->in = YES;
		if (c=='.') instr->out = YES;
		if (c=='[') instr->jump = 1;
		if (c==']') {
			instr->jump = -1;
			int dist = findMatchingBracketDist(instr);
			(instr-dist)->jump = dist;
			(instr-dist)->cond = ZERO;
			instr->jump = -dist;
			instr->cond = NONZERO;
		}
		return next;
	}
	return 0;
}

void compile(char *filename, struct Instruction *program) {
	int c;
	FILE *file = fopen(filename, "r");
	if (file == NULL) return;
	while ((c = fgetc(file)) != EOF) program += storeInstruction(c, program);
	program->end = YES;
	fclose(file);
}

void dissassemble(struct Instruction *program, char *filename) {
	FILE *file = fopen(filename, "w");
	if (file == NULL) return;
	do {
		if (program->in == YES) fputc(',', file); 
		if (program->out == YES) fputc('.', file); 
		if (program->math >= 0) for (int i = 0; i < program->math; i++) fputc('+', file); 
		else for (int i = 0; i < -program->math; i++) fputc('-', file); 
		if (program->seek >= 0) for (int i = 0; i < program->seek; i++) fputc('>', file); 
		else for (int i = 0; i < -program->seek; i++) fputc('<', file); 
		if (program->jump > 0) fputc('[', file); 
		else if (program->jump < 0) fputc(']', file); 
		fputc('\n', file); 
	} while ((program++)->end == NO);
	fclose(file);
}

void interpret(struct Instruction *program, unsigned char *tape) {
	do {
		if (program->out == YES) putc(*tape, stdout); 
		if (program->in == YES) *tape = getc(stdin);
		(*tape) += program->math;
		tape += program->seek;
		if (program->cond == ZERO && *tape == 0 || program->cond == NONZERO && *tape != 0) program += program->jump;
	} while ((program++)->end == NO);
}

unsigned char tape[65536];
struct Instruction program[65536];

int main(int argc, char *argv[]) {
	if (argc > 1) {
		compile(argv[1], program);
		tty_raw_mode();
		
		clock_t start = clock();
		interpret(program, tape);
		clock_t end = clock();
		printf("\nTook %lu clock milliseconds\n", (end - start) * 1000 / CLOCKS_PER_SEC);
	}
	
	return 0;
}
