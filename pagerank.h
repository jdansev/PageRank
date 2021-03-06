#ifndef PAGERANK_H
#define PAGERANK_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <stdbool.h>

#define EPSILON 5e-3
#define MAX_NAME 21
#define MAX_LINE 100

typedef struct page page;
typedef struct node node;
typedef struct config config;

struct page {
	char name[MAX_NAME];
	size_t index;
	size_t noutlinks;
	node* inlinks;
};

struct node {
	page* page;
	node* next;
};

struct config {
	node* list;
	size_t npages;
	size_t nedges;
	size_t nthreads;
	double dampener;
};

/* Releases dynamic memory used by list of inlinks */
void release_inlinks(node* head) {

	node* cursor = head;

	while (cursor) {
		node* next = cursor->next;
		free(cursor);
		cursor = next;
	}
}

/* Releases all dynamically allocated memory */
void release(node* head) {

	node* cursor = head;

	while (cursor) {
		node* next = cursor->next;
		release_inlinks(cursor->page->inlinks);
		free(cursor->page);
		free(cursor);
		cursor = next;
	}
}

/* Initialises the config structure with the inputted data */
void init(config* conf, int argc, char** argv) {

	size_t npages;
	size_t nedges;
	size_t nthreads;

	double dampener;

	node* head = NULL;
	node* tail = NULL;

	char line[MAX_LINE];

	// Check command line arguments
	if (argc != 2) {
		puts("Usage: pagerank <threads>");
		exit(1);
	}

	// Set number of threads
	if ((nthreads = atoi(argv[1])) < 1) {
		puts("Invalid number of threads");
		exit(1);
	}

	// Set dampening factor
	if (fgets(line, MAX_LINE, stdin) == NULL
		|| sscanf(line, "%lf", &dampener) != 1
		|| dampener < 0 || dampener > 1) {
		puts("Invalid dampening factor");
		exit(1);
	}

	// Set number of pages
	if (fgets(line, MAX_LINE, stdin) == NULL
		|| sscanf(line, "%zu", &npages) != 1
		|| npages == 0) {
		puts("Invalid number of pages");
		exit(1);
	}

	// Create hash table for fast lookups
	hcreate(npages);

	// Construct each page
	for (size_t i = 0; i < npages; i++) {

		char name[MAX_NAME];

		// Attempt to read the page name
		if (fgets(line, MAX_LINE, stdin) == NULL
			|| sscanf(line, "%20s", name) != 1) {
			puts("Unexpected EOF while reading page names");
			exit(1);
		}

		// Create the page
		page* p = malloc(sizeof(page));
		p->index = i;
		p->noutlinks = 0;
		p->inlinks = NULL;
		strcpy(p->name, name);

		// Create the page node
		node* n = malloc(sizeof(node));
		n->page = p;
		n->next = NULL;

		// Append the node to the list of pages
		if (!head) {
			head = n;
			tail = n;
		} else {
			tail->next = n;
			tail = n;
		}

		// Attempt to add the page to the hash table
		ENTRY e;
		e.key = strdup(name);
		e.data = (void*) p;

		// Ensure the page was added to the hash table
		if (!hsearch(e, ENTER)) {
			puts("Unable to insert into the hash table");
			exit(1);
		}
	}

	// Set number of edges
	if (fgets(line, MAX_LINE, stdin) == NULL
		|| sscanf(line, "%zu", &nedges) != 1
		|| nedges == 0) {
		puts("Invalid number of edges");
		exit(1);
	}

	// Construct each edge
	for (size_t i = 0; i < nedges; i++) {

		char p1_name[MAX_LINE];
		char p2_name[MAX_LINE];

		// Read the two page names for the edge
		if (fgets(line, MAX_LINE, stdin) == NULL
			|| sscanf(line, "%20s %20s", p1_name, p2_name) != 2) {
			puts("Invalid edge");
			exit(1);
		}

		ENTRY e;
		ENTRY* ep;

		// Look up corresponding page struct for page 1
		e.key = p1_name;
		ep = hsearch(e, FIND);
		if (!ep) {
			printf("Unable to find page for %s\n", p1_name);
			exit(1);
		}

		page* p1 = (page *) ep->data;

		// Look up corresponding page struct for page 2
		e.key = p2_name;
		ep = hsearch(e, FIND);
		if (!ep) {
			printf("Unable to find page for %s\n", p2_name);
			exit(1);
		}

		page* p2 = (page *) ep->data;

		// Add page 1 as an inlink to page 2
		node* n = malloc(sizeof(node));
		n->page = p1;
		n->next = p2->inlinks;
		p2->inlinks = n;

		// Increment outlinks of page 1
		p1->noutlinks++;
	}

	#ifndef __APPLE__
	for (node* c = head; c != NULL; c = c->next) {
		ENTRY e = { .key = c->page->name };
		ENTRY* ep = hsearch(e, FIND);
		if (ep) {
			free(ep->key);
		}
	}
	#endif

	// Destroy the hash table
	hdestroy();

	// Save the config
	conf->list = head;
	conf->npages = npages;
	conf->nedges = nedges;
	conf->nthreads = nthreads;
	conf->dampener = dampener;
}

#endif
