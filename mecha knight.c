/*
 * ============================================================
 *  ARQUIVO C++: salve/compile este arquivo como Main.cpp.
 *  Coloque fundo.png na pasta de execucao do jogo (x64\Debug).
 *
 *  O que o programa faz:
 *   - Um robo em pixel art anda, pula e ataca.
 *   - Setas esquerda/direita: andar
 *   - ESPACO ou seta pra cima: pular
 *   - X: atacar
 *   - ESC: fechar o jogo
 *   - Sao 3 fases (plataformas de cores diferentes).
 *     Chegou na borda DIREITA -> vai pra fase seguinte.
 *     Chegou na borda ESQUERDA -> volta pra fase anterior.
 *
 *  COMO O PIXEL ART FUNCIONA (sem arquivos de imagem):
 *   Cada desenho do robo e uma "matriz de letras". Cada letra
 *   e uma cor ('o' = contorno, 'g' = cinza, 'c' = ciano...).
 *   No comeco do programa, transformamos essas letras em
 *   BITMAPS pequenos (12x14 pixels) e depois desenhamos eles
 *   AMPLIADOS (escala 4x) pra aparecer grande e quadradinho.
 *   Assim nao precisamos de nenhuma DLL ou imagem extra.
 *
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

 // ---------- CONSTANTES DO JOGO ----------
#define LARGURA_TELA 800
#define ALTURA_TELA  600
#define FPS          60

#define ALTURA_CHAO  100     // altura da plataforma (de baixo pra cima)
#define TOTAL_FASES  3

#define VELOCIDADE   5       // pixels que o robo anda por frame
#define GRAVIDADE    0.8f    // aumento da velocidade vertical por frame
#define FORCA_PULO  -15.0f   // negativo porque o eixo Y cresce pra BAIXO

// ---------- CONSTANTES DO SPRITE ----------
#define ROBO_LARG    12      // largura do desenho do robo (em "pixels do desenho")
#define ROBO_ALT     14      // altura do desenho do robo
#define FX_LARG      6       // largura do efeito do ataque
#define FX_ALT       8       // altura do efeito do ataque
#define ESCALA       4       // cada pixel do desenho vira 4x4 pixels na tela

// Tamanho real do robo na tela (desenho x escala)
#define LARG_PX      (ROBO_LARG * ESCALA)   // 48 pixels
#define ALT_PX       (ROBO_ALT  * ESCALA)   // 56 pixels

// ---------- CONSTANTES DE ANIMACAO ----------
#define VEL_ANIMACAO   8     // troca o frame de andar a cada 8 frames do jogo
#define TEMPO_PREPARO  12    // frames do ataque "carregando" (soco recuado)
#define DURACAO_ATAQUE 24    // duracao total do ataque (preparo + golpe)

/*
 
 *  DESENHOS DO ROBO (cada linha de texto = uma linha de pixels)
 
 *  Legenda das letras:
 *    .  transparente (nao desenha nada)
 *    o  contorno escuro
 *    g  cinza (metal)
 *    w  branco (brilho)
 *    c  ciano (olhos e peito)
 *    r  vermelho (antena)
 *    y  amarelo (energia do ataque)
 
 *  IMPORTANTE: toda linha precisa ter exatamente 12 letras!
 
 */

 // A cabeca e igual na maioria dos frames, entao usamos uma
 // macro pra nao repetir. A macro e substituida pelas 6 linhas
 // abaixo (linhas 0 a 5 do desenho).
#define CABECA \
    ".....r......", \
    "..oooooooo..", \
    "..oggggggo..", \
    "..ogcggcgo..", \
    "..oggggggo..", \
    "..oooooooo.."

//  Frame: PARADO 
const char* FRAME_PARADO[ROBO_ALT] = {
    CABECA,
    ".ooggggggoo.",
    "ogoggwwggogo",
    "ogoggccggogo",
    "ogoggggggogo",
    "oo.oooooo.oo",
    "..ogo..ogo..",
    "..ogo..ogo..",
    ".oooo..oooo."
};

//  Frames de ANDAR (3 desenhos diferentes das pernas) 
// Andar A: pernas abertas
const char* FRAME_ANDAR_A[ROBO_ALT] = {
    CABECA,
    ".ooggggggoo.",
    "ogoggwwggogo",
    "ogoggccggogo",
    "ogoggggggogo",
    "oo.oooooo.oo",
    ".ogo....ogo.",
    ".ogo....ogo.",
    "oooo....oooo"
};

// Andar B: pernas juntas
const char* FRAME_ANDAR_B[ROBO_ALT] = {
    CABECA,
    ".ooggggggoo.",
    "ogoggwwggogo",
    "ogoggccggogo",
    "ogoggggggogo",
    "oo.oooooo.oo",
    "...ogoogo...",
    "...ogoogo...",
    "..oooooooo.."
};

// Andar C: perna direita levantada
const char* FRAME_ANDAR_C[ROBO_ALT] = {
    CABECA,
    ".ooggggggoo.",
    "ogoggwwggogo",
    "ogoggccggogo",
    "ogoggggggogo",
    "oo.oooooo.oo",
    "..ogo..ogo..",
    "..ogo...o...",
    "..ooo......."
};

//  Frame: PULO (bracos pra cima, pernas encolhidas) 
// Aqui a cabeca e diferente (tem bracos ao lado), por isso
// nao usamos a macro CABECA.
const char* FRAME_PULO[ROBO_ALT] = {
    ".....r......",
    "..oooooooo..",
    "..oggggggo..",
    "..ogcggcgo..",
    "ogoggggggogo",
    "ogoooooooogo",
    ".ooggggggoo.",
    "..oggwwggo..",
    "..oggccggo..",
    "..oggggggo..",
    "..oooooooo..",
    "..ogo..ogo..",
    "..ooo..ooo..",
    "............"
};

// ----- Frames de ATAQUE -----
// Ataque 1: preparando o soco (mao direita brilhando em amarelo)
const char* FRAME_ATAQUE_1[ROBO_ALT] = {
    CABECA,
    ".ooggggggoo.",
    "ogoggwwggogy",
    "ogoggccggoyy",
    "ogoggggggogy",
    "oo.oooooo.oo",
    "..ogo..ogo..",
    "..ogo..ogo..",
    ".oooo..oooo."
};

// Ataque 2: soco com o braco esticado (e pernas abertas)
const char* FRAME_ATAQUE_2[ROBO_ALT] = {
    CABECA,
    ".ooggggggooo",
    "ogoggwwgggyy",
    "ogoggccggooo",
    "ogoggggggo..",
    "oo.oooooo...",
    ".ogo....ogo.",
    ".ogo....ogo.",
    "oooo....oooo"
};

//  Efeito do ataque (onda de energia na frente do soco) 
// Este desenho tem 6 colunas x 8 linhas
const char* FRAME_EFEITO[FX_ALT] = {
    "y.....",
    "yy....",
    "wyy...",
    "wwyy..",
    "wwyy..",
    "wyy...",
    "yy....",
    "y....."
};

/*
 * Funcao: cor_do_pixel
 * Recebe uma letra do desenho e devolve a cor correspondente.
 * Se a letra nao for nenhuma das cores, devolve transparente.
 */
ALLEGRO_COLOR cor_do_pixel(char letra) {
    switch (letra) {
    case 'o': return al_map_rgb(15, 15, 25);      // contorno
    case 'g': return al_map_rgb(170, 175, 190);   // cinza
    case 'w': return al_map_rgb(235, 240, 250);   // branco
    case 'c': return al_map_rgb(0, 230, 255);     // ciano
    case 'r': return al_map_rgb(230, 50, 50);     // vermelho
    case 'y': return al_map_rgb(255, 210, 0);     // amarelo
    default:  return al_map_rgba(0, 0, 0, 0);     // transparente
    }
}

/*
 * Funcao: criar_sprite
 * Transforma um desenho feito de letras em um BITMAP do Allegro.
 *   linhas: o vetor de strings (o desenho)
 *   larg, alt: tamanho do desenho em pixels
 */
ALLEGRO_BITMAP* criar_sprite(const char* linhas[], int larg, int alt) {
    ALLEGRO_BITMAP* bmp = al_create_bitmap(larg, alt);
    if (!bmp) {
        return NULL;
    }

    // Faz o Allegro desenhar DENTRO do bitmap (e nao na tela)
    al_set_target_bitmap(bmp);

    // Comeca com tudo transparente
    al_clear_to_color(al_map_rgba(0, 0, 0, 0));

    // Percorre o desenho linha por linha, letra por letra
    for (int l = 0; l < alt; l++) {
        for (int c = 0; c < larg; c++) {
            // linhas[l][c] e a letra na linha l, coluna c
            al_put_pixel(c, l, cor_do_pixel(linhas[l][c]));
        }
    }

    return bmp;
}

/*
 * Funcao: criar_retangulo
 * Cria um bitmap de uma cor so (usado pro chao das fases).
 */
ALLEGRO_BITMAP* criar_retangulo(int largura, int altura, ALLEGRO_COLOR cor) {
    ALLEGRO_BITMAP* bmp = al_create_bitmap(largura, altura);
    if (!bmp) {
        return NULL;
    }
    al_set_target_bitmap(bmp);
    al_clear_to_color(cor);
    return bmp;
}

int main() {

    //  INICIALIZACAO DO ALLEGRO 
    if (!al_init()) {
        printf("Erro ao iniciar o Allegro\n");
        return -1;
    }
    if (!al_init_image_addon()) {
        printf("Erro ao iniciar o complemento de imagens do Allegro\n");
        return -1;
    }

    if (!al_install_keyboard()) {
        printf("Erro ao instalar o teclado\n");
        return -1;
    }

    ALLEGRO_DISPLAY* janela = al_create_display(LARGURA_TELA, ALTURA_TELA);
    if (!janela) {
        printf("Erro ao criar a janela\n");
        return -1;
    }

    //  CARREGANDO O FUNDO 
    // A imagem deve se chamar exatamente pixil-frame-0.png.
    // O programa tenta os caminhos mais comuns do Visual Studio.
    ALLEGRO_BITMAP* fundo = al_load_bitmap("pixil-frame-0.png");

    if (!fundo) {
        fundo = al_load_bitmap("x64/Debug/pixil-frame-0.png");
    }

    if (!fundo) {
        fundo = al_load_bitmap("mecha knight/x64/Debug/pixil-frame-0.png");
    }

    if (!fundo) {
        fprintf(stderr, "ERRO: nao foi possivel carregar pixil-frame-0.png.\\n");
        fprintf(stderr, "Verifique o diretorio de trabalho do Visual Studio e a pasta da imagem.\\n");
        al_destroy_display(janela);
        return -1;
    }

    printf("Imagem de fundo carregada com sucesso!\\n");

    // Timer: "bate" FPS vezes por segundo (1 batida = 1 frame)
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);

    // Fila de eventos
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    al_register_event_source(fila, al_get_display_event_source(janela));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_keyboard_event_source());

    //  CRIANDO OS BITMAPS
    // Chao de cada fase: [0] verde, [1] azul, [2] vermelho
    ALLEGRO_BITMAP* chao[TOTAL_FASES];
    chao[0] = criar_retangulo(LARGURA_TELA, ALTURA_CHAO, al_map_rgb(0, 160, 0));
    chao[1] = criar_retangulo(LARGURA_TELA, ALTURA_CHAO, al_map_rgb(0, 100, 220));
    chao[2] = criar_retangulo(LARGURA_TELA, ALTURA_CHAO, al_map_rgb(200, 60, 60));

    // Sprites do robo (cada um criado a partir das letras la de cima)
    ALLEGRO_BITMAP* spr_parado = criar_sprite(FRAME_PARADO, ROBO_LARG, ROBO_ALT);
    ALLEGRO_BITMAP* spr_pulo = criar_sprite(FRAME_PULO, ROBO_LARG, ROBO_ALT);
    ALLEGRO_BITMAP* spr_ataque[2];
    spr_ataque[0] = criar_sprite(FRAME_ATAQUE_1, ROBO_LARG, ROBO_ALT);
    spr_ataque[1] = criar_sprite(FRAME_ATAQUE_2, ROBO_LARG, ROBO_ALT);

    // Os 3 desenhos diferentes de andar
    ALLEGRO_BITMAP* spr_andar[3];
    spr_andar[0] = criar_sprite(FRAME_ANDAR_A, ROBO_LARG, ROBO_ALT);
    spr_andar[1] = criar_sprite(FRAME_ANDAR_B, ROBO_LARG, ROBO_ALT);
    spr_andar[2] = criar_sprite(FRAME_ANDAR_C, ROBO_LARG, ROBO_ALT);

    // Efeito do ataque (tem tamanho diferente do robo)
    ALLEGRO_BITMAP* spr_efeito = criar_sprite(FRAME_EFEITO, FX_LARG, FX_ALT);

    // Depois de criar tudo, voltamos a desenhar na TELA.
    // (as funcoes acima deixam o "alvo" apontando pro ultimo bitmap)
    al_set_target_backbuffer(janela);

    // Confere se tudo foi criado direitinho
    if (!chao[0] || !chao[1] || !chao[2] || !spr_parado || !spr_pulo ||
        !spr_ataque[0] || !spr_ataque[1] || !spr_andar[0] ||
        !spr_andar[1] || !spr_andar[2] || !spr_efeito) {
        printf("Erro ao criar os bitmaps\n");
        return -1;
    }

    // Ordem da animacao de andar: A -> B -> C -> B -> A ...
    // (cada numero e o indice do vetor spr_andar)
    int seq_andar[4] = { 0, 1, 2, 1 };

    ALLEGRO_COLOR cor_fundo = al_map_rgb(45, 45, 80);

    // ---------- VARIAVEIS DO JOGO ----------
    int fase = 0;                 // fase atual (0, 1 ou 2)

    float x = 50;                 // posicao do robo (canto superior esquerdo)
    float y_chao = ALTURA_TELA - ALTURA_CHAO;   // Y onde comeca o chao
    float y = y_chao - ALT_PX;    // comeca em cima do chao

    float vel_y = 0;              // velocidade vertical
    bool no_chao = true;          // esta apoiado no chao?
    bool virado_direita = true;   // pra qual lado o robo esta olhando?

    bool andando = false;         // esta andando neste frame?
    int contador_anim = 0;        // conta frames pra trocar o desenho de andar

    bool atacando = false;        // esta no meio de um ataque?
    int tempo_ataque = 0;         // quantos frames se passaram desde o inicio do ataque

    bool desenhar = false;        // so redesenha quando o timer bater
    bool rodando = true;         // controla o loop principal

    al_start_timer(timer);

    // ---------- LOOP PRINCIPAL ----------
    while (rodando) {

        ALLEGRO_EVENT evento;
        al_wait_for_event(fila, &evento);

        // --- Clicou no X da janela ---
        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }

        // --- Uma tecla foi apertada (evento unico, nao fica repetindo) ---
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {

            int tecla = evento.keyboard.keycode;

            if (tecla == ALLEGRO_KEY_ESCAPE) {
                rodando = false;
            }

            // Pulo: so no chao e fora de ataque
            if ((tecla == ALLEGRO_KEY_SPACE || tecla == ALLEGRO_KEY_UP)
                && no_chao && !atacando) {
                vel_y = FORCA_PULO;
                no_chao = false;
            }

            // Ataque: so comeca se nao estiver atacando ja
            if (tecla == ALLEGRO_KEY_X && !atacando) {
                atacando = true;
                tempo_ataque = 0;   // zera o cronometro do ataque
            }
        }

        // --- Batida do timer: atualiza a logica do jogo ---
        else if (evento.type == ALLEGRO_EVENT_TIMER) {

            // Estado atual do teclado (teclas seguradas)
            ALLEGRO_KEYBOARD_STATE teclado;
            al_get_keyboard_state(&teclado);

            // Comeca assumindo que nao esta andando; se apertar seta, muda
            andando = false;

            // So anda se NAO estiver atacando
            if (!atacando) {
                if (al_key_down(&teclado, ALLEGRO_KEY_RIGHT)) {
                    x += VELOCIDADE;
                    virado_direita = true;
                    andando = true;
                }
                if (al_key_down(&teclado, ALLEGRO_KEY_LEFT)) {
                    x -= VELOCIDADE;
                    virado_direita = false;
                    andando = true;
                }
            }

            // Contador da animacao de andar: so avanca se estiver andando
            if (andando) {
                contador_anim++;
            }
            else {
                contador_anim = 0;
            }

            // Cronometro do ataque: conta os frames e termina no final
            if (atacando) {
                tempo_ataque++;
                if (tempo_ataque >= DURACAO_ATAQUE) {
                    atacando = false;
                }
            }

            // Gravidade
            vel_y += GRAVIDADE;
            y += vel_y;

            // Colisao com o chao
            if (y + ALT_PX >= y_chao) {
                y = y_chao - ALT_PX;
                vel_y = 0;
                no_chao = true;
            }

            // ----- Troca de fase pela ESQUERDA -----
            if (x < 0) {
                if (fase > 0) {
                    fase--;                        // vai pra fase anterior
                    x = LARGURA_TELA - LARG_PX;    // aparece na borda direita
                }
                else {
                    x = 0;                         // fase 1: nao tem fase antes, trava
                }
            }

            //  Troca de fase pela DIREITA 
            // Usamos ">" (e nao ">=") pra nao trocar de fase de novo
            // logo que o robo aparece na borda direita.
            if (x + LARG_PX > LARGURA_TELA) {
                if (fase < TOTAL_FASES - 1) {
                    fase++;                        // vai pra proxima fase
                    x = 0;                         // aparece na borda esquerda
                }
                else {
                    x = LARGURA_TELA - LARG_PX;    // ultima fase: trava
                }
            }

            desenhar = true;
        }

        //  DESENHO 
        if (desenhar && al_is_event_queue_empty(fila)) {
            desenhar = false;

            al_draw_scaled_bitmap(
                fundo,
                0, 0,
                al_get_bitmap_width(fundo),
                al_get_bitmap_height(fundo),
                0, 0,
                LARGURA_TELA, ALTURA_TELA,
                0
            );

            // Chao da fase atual
            // O chao agora e o proprio cenario da imagem de lava.

            //  Escolhe qual desenho do robo usar (ordem de prioridade) 
            ALLEGRO_BITMAP* sprite_atual;

            if (atacando) {
                // Primeira metade do ataque = preparo; segunda = golpe
                if (tempo_ataque < TEMPO_PREPARO) {
                    sprite_atual = spr_ataque[0];
                }
                else {
                    sprite_atual = spr_ataque[1];
                }
            }
            else if (!no_chao) {
                sprite_atual = spr_pulo;
            }
            else if (andando) {
                // contador_anim / VEL_ANIMACAO muda de numero a cada 8 frames;
                // "% 4" faz voltar pro inicio da sequencia (0,1,2,3,0,1,...)
                int passo = (contador_anim / VEL_ANIMACAO) % 4;
                sprite_atual = spr_andar[seq_andar[passo]];
            }
            else {
                sprite_atual = spr_parado;
            }

            // Se o robo olha pra esquerda, espelhamos o desenho
            int flags = virado_direita ? 0 : ALLEGRO_FLIP_HORIZONTAL;

            // Desenha o robo AMPLIADO:
            //  origem no bitmap: (0,0) ate (ROBO_LARG, ROBO_ALT)
            //  destino na tela:  (x,y) com tamanho (LARG_PX, ALT_PX)
            al_draw_scaled_bitmap(sprite_atual,
                0, 0, ROBO_LARG, ROBO_ALT,
                x, y, LARG_PX, ALT_PX,
                flags);

            // Efeito do golpe: so aparece na fase do soco esticado
            if (atacando && tempo_ataque >= TEMPO_PREPARO) {
                // Se olha pra direita, o efeito fica na frente (a direita);
                // se olha pra esquerda, fica a esquerda do robo.
                float fx_x;
                if (virado_direita) {
                    fx_x = x + LARG_PX;
                }
                else {
                    fx_x = x - FX_LARG * ESCALA;
                }
                float fx_y = y + 4 * ESCALA;   // na altura do punho

                al_draw_scaled_bitmap(spr_efeito,
                    0, 0, FX_LARG, FX_ALT,
                    fx_x, fx_y,
                    FX_LARG * ESCALA, FX_ALT * ESCALA,
                    flags);
            }

            al_flip_display();
        }
    }

    //  LIMPEZA 
    // Destroi tudo que foi criado, pra nao vazar memoria
    for (int i = 0; i < TOTAL_FASES; i++) {
        al_destroy_bitmap(chao[i]);
    }
    for (int i = 0; i < 3; i++) {
        al_destroy_bitmap(spr_andar[i]);
    }
    al_destroy_bitmap(spr_ataque[0]);
    al_destroy_bitmap(spr_ataque[1]);
    al_destroy_bitmap(spr_parado);
    al_destroy_bitmap(spr_pulo);
    al_destroy_bitmap(spr_efeito);

    al_destroy_bitmap(fundo);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(janela);

    return 0;
}
