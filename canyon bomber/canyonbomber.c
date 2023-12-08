#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

//-------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Macros e variáveis globais do programa -----------------------------------------------------------

// Gerais
const float FPS = 75;
const int SCREEN_W = 960;
const int SCREEN_H = 540;
const int N_VIDAS = 3;

// Tiro da nave
const int RAIO_TIRO = 3;
const float ACEL_TIRO = 0.05;

// Posição da nave
const int POS_NAVE_CIMA = 40;
const int POS_NAVE_BAIXO = 100;
const float VEL_NAVE_RAPIDA = 4;
const float VEL_NAVE_LENTA = 2;

// Blocos (alvos)
const int N_LINHAS = 8;
#define N_BLOCOS_LINHA SCREEN_W / 15 // Se SCREEN_W for 960, serão 64 blocos por linha
#define BLOCO_W SCREEN_W / 64 // == 15px
#define BLOCO_H SCREEN_W / 64 // Para ter as proporções de um quadrado (nesse caso: 15 x 15)

//-------------------------------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------- Structs do programa -------------------------------------------------------------------

// Struct do tiro
typedef struct tiro {

	float x, y;
	int ativo;
	int hit;
	float vel_y;

} Ttiro;

// Struct da nave
typedef struct nave {

	ALLEGRO_BITMAP *img;
	ALLEGRO_BITMAP *img2;

	ALLEGRO_COLOR cor;
	Ttiro tiro;

	int direcao;
	float x, y;
	float vel;
    float W, H;
	int vida;
	int pontos; // Ao destruir blocos a nave do jogador ganha pontos
	int win; // 1 quando o jogador ganha, 0 caso contrário
	
} Tnave;

// Struct dos alvos da matriz de blocos
typedef struct bloco {

	ALLEGRO_COLOR cor;

	int x, y;
    int linha;
    int pontos;
    int ativo;

} Tbloco;

// Struct dos icones visuais (p1/p2/vidas)
typedef struct player_icons {

	ALLEGRO_BITMAP *img_vida;
	int vida_W, vida_H;

	ALLEGRO_BITMAP *img_p1;
	int p1_W, p1_H;

	ALLEGRO_BITMAP *img_p2;
	int p2_W, p2_H;

} Tplayer_icons;

// Struct de texto
typedef struct text {

	ALLEGRO_FONT *fonte_small;
	ALLEGRO_FONT *fonte_big;
	ALLEGRO_COLOR cor;
	ALLEGRO_COLOR cor2;

} Ttext;

//-------------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------- Funções do programa --------------------------------------------------------------------


// Funções mais gerais

int init_mod_al (ALLEGRO_TIMER **timer, ALLEGRO_DISPLAY **display, ALLEGRO_EVENT_QUEUE **event_queue) {
	// Inicializa o Allegro. Se der um erro, retorna uma mensagem de erro
	if (!al_init ()) {
		fprintf (stderr, "Failed to initialize allegro!\n");
		return -1;
	}

	// Inicializa o módulo de primitivas do Allegro. Se der um erro, retorna uma mensagem de erro
    if (!al_init_primitives_addon ()) {
		fprintf (stderr, "Failed to initialize primitives!\n");
        return -1;
    }

	// Inicializa o modulo que permite carregar imagens no jogo
    al_init_image_addon ();
    if (!al_init_image_addon ()) {
        fprintf (stderr, "Failed to initialize image module!\n");
        return -1;
    }

	// Instala o teclado. Se der um erro, retorna uma mensagem de erro
	if (!al_install_keyboard ()) {
		fprintf (stderr, "Failed to install keyboard!\n");
		return -1;
	}

	// Inicializa o modulo allegro que carrega as fontes
	al_init_font_addon ();

	// Inicializa o modulo allegro que entende arquivos tff de fontes. Se der um erro, retorna uma mensagem de erro
	if (!al_init_ttf_addon ()) {
		fprintf (stderr, "Failed to load tff font module!\n");
		return -1;
	}

	*timer = al_create_timer (1.0 / FPS); // Temporizador incrementa uma unidade a cada 1.0/FPS segundos
	if (!timer) {
		fprintf (stderr, "Failed to create timer!\n");
		return -1;
	}

	*display = al_create_display (SCREEN_W, SCREEN_H);
	if (!(*display)) {
		fprintf (stderr, "Failed to create display!\n");
		al_destroy_timer (*timer);
		return -1;
	}

	*event_queue = al_create_event_queue ();
	if (!(*event_queue)) {
		fprintf (stderr, "Failed to create event_queue!\n");
        al_destroy_timer (*timer);
		al_destroy_display (*display);
		return -1;
	}

	return 0;
}
void encerra_mod_al (ALLEGRO_TIMER *timer, ALLEGRO_DISPLAY *display, ALLEGRO_EVENT_QUEUE *event_queue, ALLEGRO_BITMAP *background) {
	
	if (timer != NULL) { al_destroy_timer (timer); }
	if (display != NULL) { al_destroy_display (display); }
	if (event_queue != NULL) { al_destroy_event_queue (event_queue); }
	if (background != NULL) { al_destroy_bitmap (background); }
}
void encerra_player_bitmaps (Tnave *nave1, Tnave *nave2, Tplayer_icons *iconesp1p2) {
	al_destroy_bitmap (nave1->img);
	al_destroy_bitmap (nave1->img2);

	al_destroy_bitmap (nave2->img);
	al_destroy_bitmap (nave2->img2);

	if (iconesp1p2->img_vida != NULL) { al_destroy_bitmap (iconesp1p2->img_vida); }
	if (iconesp1p2->img_p1 != NULL) { al_destroy_bitmap (iconesp1p2->img_p1); }
	if (iconesp1p2->img_p2 != NULL) { al_destroy_bitmap (iconesp1p2->img_p2); }
}
int init_text (Ttext *txt) {
	txt->cor = al_map_rgb (255, 255, 255); // Branco
	txt->cor2 = al_map_rgb (188, 0, 203); // Roxo claro

	txt->fonte_small = al_load_font ("./fontes/DigitalDisco.ttf", 45, 0);
    if (!txt->fonte_small) {
        fprintf (stderr, "Failed to load the text font!\n");
        return -1;
    }

	txt->fonte_big = al_load_font ("./fontes/DigitalDisco.ttf", 150, 0);
    if (!txt->fonte_big) {
        fprintf (stderr, "Failed to load the text font!\n");
        return -1;
    }

	return 0;
}


// Funções relativas aos arquivos

int ler_vitorias (int *vitorias_p1, int *vitorias_p2) {
    FILE *arquivo_p1 = fopen ("./vitorias/vitorias_p1.txt", "r");
    FILE *arquivo_p2 = fopen ("./vitorias/vitorias_p2.txt", "r");

    if (arquivo_p1 != NULL && arquivo_p2 != NULL) {

        fscanf (arquivo_p1, "%d", vitorias_p1);
        fscanf (arquivo_p2, "%d", vitorias_p2);

        fclose (arquivo_p1);
        fclose (arquivo_p2);

    } else {
		printf ("\nFuncao ler_arquivos: Erro ao abrir o(s) arquivo(s) de recorde do(s) jogador(es)!\n");
		return -1;
	}

	return 0;
}
void salvar_vitorias (int vitorias_p1, int vitorias_p2, Tnave p1, Tnave p2) {

	// P1 ganhou!
	if (p1.win == 1) {
		
		FILE *arquivo_p1 = fopen ("./vitorias/vitorias_p1.txt", "w");

		if (arquivo_p1 != NULL) {
			fprintf (arquivo_p1, "%d", vitorias_p1);
			fclose (arquivo_p1);
		} else {
			printf ("\nFuncao salvar_vitorias: Erro ao abrir o arquivo de recorde do jogador 1!\n");
			printf ("O numero de vitorias nao foi atualizado nessa partida!\n");
		}

	// P2 ganhou !
	} else if (p2.win == 1) {
		
		FILE *arquivo_p2 = fopen ("./vitorias/vitorias_p2.txt", "w");

		if (arquivo_p2 != NULL) {
			fprintf (arquivo_p2, "%d", vitorias_p2);
			fclose (arquivo_p2);
		} else {
			printf ("\nFuncao salvar_vitorias: Erro ao abrir o arquivo de recorde do jogador 2!\n");
			printf ("O numero de vitorias nao foi atualizado nessa partida!\n");
		}
	}
}


// Funções de gameover

void checar_gameover (Tnave p1, Tnave p2, int *playing, int all_grid_destroyed) {

	if (p1.vida == 0) {

		printf ("\n-----> [ GAME OVER: p1 perdeu! ]");
		*playing = 0; // Para encerrar o loop de jogo

	} else if (p2.vida == 0) {

		printf ("\n-----> [ GAME OVER: p2 perdeu! ]");
		*playing = 0; // Para encerrar o loop de jogo

	} else if (all_grid_destroyed == 1) {

		printf ("\n-----> [ GAME OVER: grid destruído! ]");
		*playing = 0; // Para encerrar o loop de jogo

	}
}
int tela_gameover (Ttext *txt, Tnave p1, Tnave p2, int vitorias_p1, int vitorias_p2) {

	// Coordenadas x, y do ponto central da tela

	int pos_x = SCREEN_W / 2, pos_y = (SCREEN_H / 2) - 50;

	// Strings para armazenar o valor convertido de pontos

	int pts_p1 = p1.pontos, pts_p2 = p2.pontos;
	char pts_p1_str [50], pts_p2_str [50]; // Strings pro número de pontos
	char vct_p1_str [50], vct_p2_str [50]; // Strings pro número de vitórias

	// Printa o vencedor da partida atual

	al_draw_text (txt->fonte_big, txt->cor, pos_x, pos_y - 180, ALLEGRO_ALIGN_CENTER, "GAME OVER");
	if (p1.win == 1) {
		al_draw_text (txt->fonte_small, txt->cor2, pos_x, pos_y, ALLEGRO_ALIGN_CENTER, "Vencedor: p1");
	}
	else if (p2.win == 1) {
		al_draw_text (txt->fonte_small, txt->cor2, pos_x, pos_y, ALLEGRO_ALIGN_CENTER, "Vencedor: p2");
	}

	// Converte o valor de pontos do p1 para string e printa na tela. Printa total de vitórias do p1 também

    sprintf (pts_p1_str, "Pontos p1: %d", pts_p1);
	al_draw_text (txt->fonte_small, txt->cor, pos_x, pos_y + 50, ALLEGRO_ALIGN_CENTER, pts_p1_str);

	sprintf (vct_p1_str, "Total de vitorias p1: %d", vitorias_p1);
	al_draw_text (txt->fonte_small, txt->cor, pos_x, pos_y + 90, ALLEGRO_ALIGN_CENTER, vct_p1_str);

	// Converte o valor de pontos do p2 para string e printa na tela. Printa total de vitórias do p2 também

	sprintf (pts_p2_str, "Pontos p2: %d", pts_p2);
	al_draw_text (txt->fonte_small, txt->cor, pos_x, pos_y + 190, ALLEGRO_ALIGN_CENTER, pts_p2_str);

	sprintf (vct_p2_str, "Total de vitorias p2: %d", vitorias_p2);
	al_draw_text (txt->fonte_small, txt->cor, pos_x, pos_y + 230, ALLEGRO_ALIGN_CENTER, vct_p2_str);

	return 0;
}
void confere_vencedor (Tnave *p1, Tnave *p2, int *vitorias_p1, int *vitorias_p2 , int all_grid_destroyed) {

	// Atualiza a prop ".win" do vencedor e incrementa o número de vitórias

	// Primeiro confere se a vida de um dos jogadores zerou
	if (p1->vida == 0 && p2->vida > 0) {

		p2->win = 1; // Vitória do p2
		*vitorias_p2 += 1;
	} else if (p2->vida == 0 && p1->vida > 0) {

		p1->win = 1; // Vitória do p1
		*vitorias_p1 += 1;
	} 
	

	// Caso ambos jogadores ainda tenham vida > 0, confere se o grid foi todo destruído e quem tem mais pontos
	else if ((p2->pontos > p1->pontos) && (all_grid_destroyed == 1)) {

		p2->win = 1; // Vitória do p2
		*vitorias_p2 += 1;
	}
	else if ((p1->pontos > p2->pontos && all_grid_destroyed == 1)) {

		p1->win = 1; // Vitória do p1
		*vitorias_p1 += 1;
	}
}


// Funções relativas aos jogadores

void printar_pontos_e_vidas (Tnave p1, Tnave p2) {

	printf ("\n\n---------------------------------\n\n");


	printf ("Pontuacao dos jogadores:\n\n");


	printf ("Pontos do p1: %d\n", p1.pontos);
	printf ("Vida do p1: %d\n\n", p1.vida);

	printf ("Pontos do p2: %d\n", p2.pontos);
	printf ("Vida do p2: %d", p2.vida);


	printf ("\n\n---------------------------------\n\n");
}
int criar_player_icons (Tplayer_icons *var, char *nome_arq_hp, char *nome_arq_p1, char *nome_arq_p2) {

	// Carrega o ícone de vida (coração)
	var->img_vida = al_load_bitmap (nome_arq_hp);
	if (!var->img_vida) {
        fprintf (stderr, "Failed to create vida bitmap!\n");
        return -1;
    }
	var->vida_W = al_get_bitmap_width(var->img_vida);
    var->vida_H = al_get_bitmap_height(var->img_vida);


	// Carrega o ícone do p1
	var->img_p1 = al_load_bitmap (nome_arq_p1);
	if (!var->img_p1) {
        fprintf (stderr, "Failed to create p1 (icon) bitmap!\n");
        return -1;
    }
	var->p1_W = al_get_bitmap_width(var->img_p1);
    var->p1_H = al_get_bitmap_height(var->img_p1);


	// Carrega o ícone do p2
	var->img_p2 = al_load_bitmap (nome_arq_p2);
	if (!var->img_p2) {
        fprintf (stderr, "Failed to create p2 (icon) bitmap!\n");
        return -1;
    }
	var->p2_W = al_get_bitmap_width(var->img_p2);
    var->p2_H = al_get_bitmap_height(var->img_p2);

	return 0;
}
void desenhar_player_icons (Tplayer_icons *var, Tnave p1, Tnave p2) {

	int i, j;
	// Estrutura da função al_draw_bitmap: al_draw_bitmap (img, pos x, pos y, outros parâmetros opcionais)

	// Para o player 1
	int x_p1 = var->p1_W + 10;
	for (i = 0; i < (1 + p1.vida); i++) {

		// Desenha o ícone "p1" na tela
		al_draw_bitmap (var->img_p1, 10, 10, 0);

		// Desenha o número adequado de corações
		if (p1.vida > 0) {
			for (j = 0; j < p1.vida; j++) {
				al_draw_bitmap (var->img_vida, (x_p1 + (5 * p1.vida) + (j * var->vida_W)), 10, 0);
			}
		}
	}
	
	// Para o player 2
	int x_p2 = SCREEN_W - var->p2_W - 10;
	for (i = 0; i < (1 + p2.vida); i++) {

		// Desenha o ícone "p2" na tela
		al_draw_bitmap (var->img_p2, x_p2, 10, 0);

		// Desenha o número adequado de corações
		if (p2.vida > 0) {
			for (j = 1; j <= p2.vida; j++) {
				al_draw_bitmap (var->img_vida, (x_p2 - j * var->vida_W - 5 * p2.vida), 10, 0);
			}
		}
	}
}


// Funções relativas ao background e ao grid de alvos

int cria_background (ALLEGRO_BITMAP **background) {
	*background = al_load_bitmap ("./imagens/bg.png"); // OBS: a imagem está ajustada para 960 W x 540 H
	if (!(*background)) {
		fprintf (stderr, "Failed to load background image!\n");
		return -1;
	}

	return 0;
}
void init_blocos (Tbloco grid [N_LINHAS][N_BLOCOS_LINHA]) {

	int L, C; // L refere-se à linha e C à coluna

    for (L = 0; L < N_LINHAS; L++) {
        for (C = 0; C < N_BLOCOS_LINHA; C++) {

			// Define parâmetros baseados na linha (cor e pontos)

			switch (L) {
				case 0:
					grid[L][C].cor = al_map_rgb (236, 0, 255); // Roxo claro 4
					grid[L][C].pontos = 1;
					break;
				case 1:
					grid[L][C].cor = al_map_rgb (217, 0, 234); // Roxo claro 3
					grid[L][C].pontos = 2;
					break;
				case 2:
					grid[L][C].cor = al_map_rgb (188, 0, 203); // Roxo claro 2
					grid[L][C].pontos = 3;
					break;
				case 3:
					grid[L][C].cor = al_map_rgb (164, 0, 177); // Roxo claro 1
					grid[L][C].pontos = 4;
					break;
				case 4:
					grid[L][C].cor = al_map_rgb (142, 0, 153); // Roxo
					grid[L][C].pontos = 5;
					break;
				case 5:
					grid[L][C].cor = al_map_rgb (119, 0, 128); // Roxo escuro 1
					grid[L][C].pontos = 6;
					break;
				case 6:
					grid[L][C].cor = al_map_rgb (102, 0, 111); // Roxo escuro 2 
					grid[L][C].pontos = 7;
					break;
				case 7:
					grid[L][C].cor = al_map_rgb (81, 0, 87); // Roxo escuro 3
					grid[L][C].pontos = 8;
					break;
			}

			// Define parâmetros gerais

            grid [L][C].linha = L;
            grid [L][C].ativo = 1; // Inicia os blocos como ativos (1)
			grid [L][C].x = C * BLOCO_W;
            grid [L][C].y = SCREEN_H - (N_LINHAS - L) * BLOCO_H;
        }
    }
}
void desenhar_blocos (Tbloco grid [N_LINHAS][N_BLOCOS_LINHA]) {

	int L, C; // L refere-se à linha e C à coluna

    for (L = 0; L < N_LINHAS; L++) {
        for (C = 0; C < N_BLOCOS_LINHA; C++) {

			// Se o bloco estiver ativo, desenha-o na tela
			if (grid [L][C].ativo == 1) {
				al_draw_filled_rectangle (
					// Coordenadas x, y do canto superior esquerdo do bloco
					grid [L][C].x,
					grid [L][C].y,

					// Coordenadas x, y do canto inferior direito do bloco
					grid [L][C].x + BLOCO_W,
					grid [L][C].y + BLOCO_H,

					// Cor do bloco
					grid [L][C].cor
				);
			}
        }
    }
}
int conferir_destruicao_grid (Tbloco grid [N_LINHAS][N_BLOCOS_LINHA]) {
	int L, C, flag = 1;

    for (L = 0; L < N_LINHAS; L++) {
        for (C = 0; C < N_BLOCOS_LINHA; C++) {

			// Se algum bloco estiver ativo, desativa a flag (nem todo grid foi destruído)
			if (grid [L][C].ativo == 1) { flag = 0; }
        }
    }
	return flag;
}


// Funções relativas as naves e ao tiro

int cria_nave_e_tiro (Tnave *nave, int pos_x, int pos_y, char *nome_arq_nave, char *nome_arq_nave2) {
	
	// Armazena a imagem da nave (grande e pequena)

    nave->img = al_load_bitmap (nome_arq_nave);
	if (!nave->img) {
        fprintf (stderr, "Failed to create nave bitmap!\n");
        return -1;
    }
	
	nave->img2 = al_load_bitmap (nome_arq_nave2);
	if (!nave->img2) {
        fprintf (stderr, "Failed to create nave bitmap!\n");
        return -1;
    }

	// Configurações da nave

    nave->x = pos_x; nave->y = pos_y; // Posição inicial x, y da nave

	nave->W = al_get_bitmap_width (nave->img);
    nave->H = al_get_bitmap_height (nave->img);

	nave->direcao = 1; // Direção padrão, da esquerda pra direita
	nave->cor = al_map_rgb (255, 0, 0);
	nave->vel = VEL_NAVE_RAPIDA;
	nave->vida = N_VIDAS;
	nave->pontos = 0;
	nave->win = 0;
	
	// Configurações do tiro da nave

	nave->tiro.x = 0; nave->tiro.y = 0; // Posição inicial x, y do tiro
	nave->tiro.ativo = 0;
	nave->tiro.hit = 0;
	nave->tiro.vel_y = 0;

	return 0;
}
void atualiza_nave_e_tiro (Tnave *nave, int player_token, Tbloco grid [N_LINHAS][N_BLOCOS_LINHA]) {

	int i, j;

	// Atualiza a posição x da nave de acordo com a velocidade
	nave->x += nave->direcao * nave->vel;

	// Muda a velocidade e inverte a direção da nave se ela tiver ultrapassado os limites da tela
	if (nave->x > SCREEN_W) {

		nave->direcao *= -1; // Se a direção já for -1, [(-1) * (-1) = 1]
		nave->x = SCREEN_W;

		// Para o p1
		if (player_token == 1) { nave->vel = VEL_NAVE_LENTA; }

		// Para o p2
		else if (player_token == 2) { nave->vel = VEL_NAVE_RAPIDA; }
	}
	else if (nave->x < -nave->W) {

		nave->direcao *= -1;
		nave->x = -nave->W;

		// Para o p1
		if (player_token == 1) { nave->vel = VEL_NAVE_RAPIDA; }

		// Para o p2
		else if (player_token == 2) { nave->vel = VEL_NAVE_LENTA; }
	}
	
	// Atualiza a posição do tiro de acordo com a posição da nave
	if (nave->tiro.ativo) {
		nave->tiro.x = nave->x + nave->W / 2;
		nave->tiro.vel_y += ACEL_TIRO;
		nave->tiro.y += nave->tiro.vel_y;
	}
	
	// Desativa o tiro se ele tiver ultrapassado os limites da tela
	// Se não tiver acertado nada tira uma vida do jogador
	if ((nave->tiro.x < -RAIO_TIRO) || (nave->tiro.x > SCREEN_W + RAIO_TIRO) || (nave->tiro.y > SCREEN_H + RAIO_TIRO)) {
		
		// Se o tiro não tiver acertado, a vida for maior que zero e o tiro estiver ativo...
		if ((nave->tiro.hit == 0)  &&  (nave->vida > 0)  &&  (nave->tiro.ativo == 1)) {
			nave->vida -= 1;
			printf ("\n-----> p%d perdeu 1 vida! Vidas restantes: %d", player_token, nave->vida);
		}

		// Desativa o tiro
		nave->tiro.ativo = 0;
	}

	// Verifica se houve colisão do tiro com blocos da matriz
	for (i = 0; i < N_LINHAS; i++) {
		for (j = 0; j < N_BLOCOS_LINHA; j++) {
		if (
				// Tiro e bloco estão ativos
				nave->tiro.ativo && grid [i][j].ativo && 

				// Tiro está nos limites da tela do jogo
				nave->tiro.x >= 0 && nave->tiro.x <= SCREEN_W && 
				nave->tiro.y >= 0 && nave->tiro.y <= SCREEN_H &&
			
				// Cood x do tiro está dentro do range x (largura) do bloco
				nave->tiro.x >= grid [i][j].x &&
				nave->tiro.x <= grid [i][j].x + BLOCO_W &&
				
				// Cood y do tiro está dentro do range y (altura) do bloco
				nave->tiro.y >= grid [i][j].y &&
				nave->tiro.y <= grid [i][j].y + BLOCO_H
				
			) {
				grid [i][j].ativo = 0; // Desativa o bloco atingido
				nave->pontos += grid [i][j].pontos; // Adiciona os pontos do bloco destruído a nave do player
				if (nave->tiro.hit == 0) { nave->tiro.hit = 1; } // Indica que o tiro conseguiu atingir pelo menos 1 alvo
			}
		}
	}
}
void desenha_nave_e_tiro (Tnave *nave, int player_token) {

	// Desenha a nave de acordo com a direção. A lógica é invertida para o p2

	if (nave->direcao == -1) {
		if (player_token == 1) { al_draw_bitmap (nave->img2, nave->x, nave->y, ALLEGRO_FLIP_HORIZONTAL); }
		else if (player_token == 2) { al_draw_bitmap (nave->img, nave->x, nave->y, ALLEGRO_FLIP_HORIZONTAL); }
	} 
	else {
		if (player_token == 1) { al_draw_bitmap (nave->img, nave->x, nave->y, 0); }
		else if (player_token == 2) { al_draw_bitmap (nave->img2, nave->x, nave->y, 0); }
	}

	// Desenha o tiro se ele estiver ativo

	if (nave->tiro.ativo) {
		al_draw_filled_circle (nave->tiro.x, nave->tiro.y, RAIO_TIRO, nave->cor);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------- MAIN ------------------------------------------------------------------------

int main (int argc, char **argv) {
	
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_BITMAP *background = NULL;

   	//---------------------------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------------------------------- Rotinas de inicialização ----------------------------------------------------------------
    
	// Inicia vários módulos do allegro necessários para o programa e cria o background

	if (init_mod_al (&timer, &display, &event_queue) != 0) { 
		encerra_mod_al (timer, display, event_queue, background); return -1;
	}
	if (cria_background (&background) != 0) {
		encerra_mod_al (timer, display, event_queue, background); return -1;
	}

	// Inicia as fontes do jogo

	Ttext txt;
	if (init_text (&txt) != 0) {
		encerra_mod_al (timer, display, event_queue, background); return -1;
	}

	// Lê o número de vitórias atual dos jogadores e armazena nas variáveis

	int vitorias_p1 = 0, vitorias_p2 = 0;
	if (ler_vitorias (&vitorias_p1, &vitorias_p2) != 0) { 
		encerra_mod_al (timer, display, event_queue, background); return -1;
	}

	// Variável pra controlar estado de destruição do grid
	// Zero se ainda houver um ou mais blocos intactos, 1 caso contrário

	int all_grid_destroyed = 0;

	//---------------------------------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------- Criando as naves, grid de blocos, etc --------------------------------------------------------

	// Cria as naves dos jogadores
	// Estrutura da função: cria_nave_e_tiro (struct Tnave, pos x nave, pos y nave, nome arq nave pequena, nome arq nave grande)

	Tnave p1nave;
	if (cria_nave_e_tiro (&p1nave, -p1nave.W, POS_NAVE_CIMA, "./imagens/nave1.png", "./imagens/nave1b.png") != 0) { 
		encerra_mod_al (timer, display, event_queue, background); return -1;
	}
	Tnave p2nave;
	if (cria_nave_e_tiro (&p2nave, SCREEN_W - 1, POS_NAVE_BAIXO, "./imagens/nave2.png", "./imagens/nave2b.png") != 0) { 
		encerra_mod_al (timer, display, event_queue, background); return -1;
	}

	// Cria um grid de blocos com 3 linhas e x elementos por linha

	Tbloco grid_blocos [N_LINHAS][N_BLOCOS_LINHA];
	init_blocos (grid_blocos); // Atribui os valores iniciais pra cada bloco

	// Cria os ícones de vida dos jogadores

	Tplayer_icons icones_p1_e_p2;
	if (criar_player_icons (&icones_p1_e_p2, "./imagens/vida.png", "./imagens/p1.png", "./imagens/p2.png") != 0) {
		encerra_mod_al (timer, display, event_queue, background); 
		encerra_player_bitmaps (&p1nave, &p2nave, &icones_p1_e_p2);
		return -1;
	}

	//---------------------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------- Iniciando o jogo --------------------------------------------------------------------

	// Registra na fila os eventos de tela (ex: clicar no X na janela)
	al_register_event_source (event_queue, al_get_display_event_source (display));

	// Registra na fila os eventos de tempo: quando o tempo altera de t para t+1
	al_register_event_source (event_queue, al_get_timer_event_source (timer));

	// Registra na fila os eventos de teclado (ex: pressionar uma tecla)
	al_register_event_source (event_queue, al_get_keyboard_event_source ());

	// Inicia o temporizador
	al_start_timer (timer);

	// Printa a pontuação inicial (0) e as vidas (3) dos jogadores
	printar_pontos_e_vidas (p1nave, p2nave);

	int playing = 1;
	while (playing) {

		ALLEGRO_EVENT ev;
		al_wait_for_event (event_queue, &ev); // Espera por um evento e o armazena na variavel de evento ev

		// Se o tipo de evento for um evento do temporizador, ou seja, se o tempo passou de t para t+1
		if (ev.type == ALLEGRO_EVENT_TIMER) {

			// Contador de segundos do terminal
			if (al_get_timer_count (timer) % (int) FPS == 0) {
				printf ("\n%ds", (int) (al_get_timer_count (timer) / FPS));
			}

			// Checar condições de game over (zero vidas ou sem blocos)
			if (conferir_destruicao_grid (grid_blocos) != 0) { all_grid_destroyed = 1; }
			checar_gameover (p1nave, p2nave, &playing, all_grid_destroyed);

			// Desenha o cenário de fundo e as vidas dos jogadores
			al_draw_bitmap (background, 0, 0, 0);
			desenhar_player_icons (&icones_p1_e_p2, p1nave, p2nave);

			// Atualiza as coordenadas da nave/tiro
			atualiza_nave_e_tiro (&p1nave, 1, grid_blocos);
			atualiza_nave_e_tiro (&p2nave, 2, grid_blocos);

			// Desenha nave/tiro na tela
			desenha_nave_e_tiro (&p1nave, 1);
			desenha_nave_e_tiro (&p2nave, 2);
			
			// Desenhar a matriz de alvos
			desenhar_blocos (grid_blocos);
			
			// Atualiza a tela
			al_flip_display ();
		}

		// Se o tipo de evento for o fechamento da tela (clique no x da janela)
		else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			playing = 0;
		}

		// Se o tipo de evento for um pressionar de uma tecla
		else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
			switch (ev.keyboard.keycode) {

				// Tecla espaço é usada pelo p1 para atirar
				case ALLEGRO_KEY_S:
					p1nave.tiro.ativo = 1;
					p1nave.tiro.hit = 0;
					p1nave.tiro.x = p1nave.x;
					p1nave.tiro.y = p1nave.y;
					p1nave.tiro.vel_y = 0;
					printf ("\n-----> p1 atirou!");
				break;

				// Tecla espaço é usada pelo p2 para atirar
				case ALLEGRO_KEY_K:
					p2nave.tiro.ativo = 1;
					p2nave.tiro.hit = 0;
					p2nave.tiro.x = p2nave.x;
					p2nave.tiro.y = p2nave.y;
					p2nave.tiro.vel_y = 0;
					printf ("\n-----> p2 atirou!");
				break;
			}
		}
	
	} // Fim do while
    
	//---------------------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------ Tela de resultados finais ------------------------------------------------------------------
	
	// Printa (no terminal) a pontuação final dos jogadores e atualiza as vitórias dos jogadores

	printar_pontos_e_vidas (p1nave, p2nave);
	confere_vencedor (&p1nave, &p2nave, &vitorias_p1, &vitorias_p2, all_grid_destroyed);
	salvar_vitorias (vitorias_p1, vitorias_p2, p1nave, p2nave);

	// Exibe a tela final de gameover

	al_draw_bitmap (background, 0, 0, 0);
	if (tela_gameover (&txt, p1nave, p2nave, vitorias_p1, vitorias_p2) != 0) {
		encerra_player_bitmaps (&p1nave, &p2nave, &icones_p1_e_p2);
		encerra_mod_al (timer, display, event_queue, background);
		return -1;
	}
	al_flip_display ();
	al_rest (6.0);

	//---------------------------------------------------------------------------------------------------------------------------------------------------
	//--------------------------------- Procedimentos de fim de jogo (fecha a tela, limpa a memória, etc) -----------------------------------------------
	
	encerra_player_bitmaps (&p1nave, &p2nave, &icones_p1_e_p2);
	encerra_mod_al (timer, display, event_queue, background);
   
	return 0;
}