/*
   "I think the woman was born in Far Madding in a thunderstorm. She probably
    told the thunder to be quiet. It probably did."
      Basel Gill
*/


/**** BEGIN LICENSE BLOCK ****

BSD 3-Clause License

Copyright (c) 2023, the wind.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**** END LICENCE BLOCK ****/

// kspace.kss attempt. An old code of mine. TODO a lot to improve below

/*
    //D - debug code - "sed" uncomments it when I'm debuging
    add "TODONT" at
     - kate3: share/apps/katepart/syntax/alert.xml
     - kate5: ~/.local/share/org.kde.syntax-highlighting/syntax/alert.xml
              (the file isn't there; get it from syntax-highlighting-5.*)
*/

#if _WIN32
#define random rand
#define srandom srand
#endif

#define D

#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <GL/gl.h>
#include <GL/glut.h>

static struct Renderer final
{
    void Point(float const v[3], unsigned int c) const
    {//TODO group by call and/or color
        glColor4ubv ((const GLubyte *)&c),
            glBegin (GL_POINTS), glVertex3fv (v), glEnd ();
    }
    void Star(float const p[3], float const v[8*3], unsigned int color[3]) const
    {
        glBegin (GL_TRIANGLE_FAN);
            glColor4ubv ((const GLubyte *)color), glVertex3fv (p),
            glColor4ubv ((const GLubyte *)(color+2)), glVertex3fv (v),
            glColor4ubv ((const GLubyte *)(color+1)), glVertex3fv (v+3),
            glColor4ubv ((const GLubyte *)(color+2)), glVertex3fv (v+6),
            glColor4ubv ((const GLubyte *)(color+1)), glVertex3fv (v+9),
            glColor4ubv ((const GLubyte *)(color+2)), glVertex3fv (v+12),
            glColor4ubv ((const GLubyte *)(color+1)), glVertex3fv (v+15),
            glColor4ubv ((const GLubyte *)(color+2)), glVertex3fv (v+18),
            glColor4ubv ((const GLubyte *)(color+1)), glVertex3fv (v+21);
            glColor4ubv ((const GLubyte *)(color+2)), glVertex3fv (v),
        glEnd ();
    }
} R;

// glFrustum parameters
static float const FNEAR = 1.f, FFAR = 2.f, FDEPTH = FFAR - FNEAR;
static float const FLEFT = -1.f, FRIGHT = 1.f, FBOTTOM = -1.f, FTOP = 1.f;
static float const FWIDTH = FRIGHT - FLEFT, FHEIGHT = FTOP - FBOTTOM;

static float const DELTA = .0001f; // float comparison, avoid division by 0

// no need to depend on math.h
#define ROUND(F) (int)((F)+ (F > 0.f ? .5f : -.5f))
#define ABS(F) (F < 0 ? -F : F)

static bool paused = false;
static GLsizei current_viewport_width {0}, current_viewport_height {0};

// for placement new
#include <new>

static struct Space
{
    // [0;N)
    static int rnd(int N) { return (int)(random () / (RAND_MAX / N + 1)); }
    // rndf (3.14, 100) - [0;3.14), rndf (3.14, 10) - [0;3.1)
    static float rndf(float N, int k) { return rnd ((int)(N*k))/(float)k; }
    struct Star final {
        unsigned int color; // RGBA
        static const int T {3}; // trail size
        float p[3], t[3*T], _d[3]; // base pos(x,y,z), trail(fifo), delta(x,y,z)
        int w {0}, _c {0}; // t write, t read; t - inneficient and simple
        bool recompute {false};
        int size {2};
        float rz {.0f}, speed_rz {.1f}; // rotation around the Z axis
        float shape[3*8] {};
        unsigned int shape_colors[3] {0, 0, 0};
        void compute_shape() // compute when p is changed
        {
            //TODO compute when their dependencies change, only
            if (current_viewport_width <= 0 || current_viewport_height <= 0)
                return;
            float c = size*(FWIDTH/(float)current_viewport_width),
                  d = size*(FHEIGHT/(float)current_viewport_height),
                  e = c/2.f, f = d/2.f;
            shape[ 0] =p[0]-c, shape[ 1] = p[1]+d, shape[ 2] = p[2],
            shape[ 3] =p[0]-e, shape[ 4] = p[1]  , shape[ 5] = p[2],
            shape[ 6] =p[0]-c, shape[ 7] = p[1]-d, shape[ 8] = p[2],
            shape[ 9] =p[0]  , shape[10] = p[1]-f, shape[11] = p[2],
            shape[12] =p[0]+c, shape[13] = p[1]-d, shape[14] = p[2],
            shape[15] =p[0]+e, shape[16] = p[1]  , shape[17] = p[2],
            shape[18] =p[0]+c, shape[19] = p[1]+d, shape[20] = p[2],
            shape[21] =p[0]  , shape[22] = p[1]+f, shape[23] = p[2];
        }
        inline void move() //TODO accelerate - exponential
        {
            t[w] = p[0], t[w+1] = p[1], t[w+2] = p[2]; // put
            w += 3, w %= 3 * T;                        // move w ptr
            if (_c < T) _c++;                          // update count

            // allow 1 more step for x or y, so rendering can start off-screen
            if (p[0] <= -1.f || p[0] >= 1.f || p[1] <= -1.f || p[1] >= 1.f) {
                //D paused = true;
                //D printf ("x: %.2f, y: %.2f, z:%.2f\n", p[0], p[1], p[2]);
                recompute = true; // regen ();
                return;
            }

            if (p[2] > -(FNEAR-DELTA)) {//TODO mv -v . test-suite/
                printf ("bug, x: %.4f, y: %.4f, z:%.4f, dz: %.4f\n",
                        p[0], p[1], p[2], _d[2]);
                exit (1);
            }
            p[0] += _d[0], p[1] += _d[1], p[2] += _d[2];
            compute_shape ();
        }
        void render(const Renderer & rc)
        {
            if (recompute) recompute = false, regen ();

            {// Slow:
                glLoadIdentity ();
                glTranslatef (p[0], p[1], p[2]);
                glRotatef (rz, 0.f, 0.f, 1.f);
                glTranslatef (-p[0], -p[1], -p[2]);
                rz = rz + speed_rz; if (rz > 360.f) rz = 0.f;
            }

            // const unsigned int F[T+1] = {0, 0x90000000u, 0x59000000u,
            //    0x37000000u, 0x22000000u, 0x15000000u};
            // const unsigned int F[T+1] = {0, 0x90000000u,
            //    0x37000000u, 0x15000000u};
            const unsigned int F[T+1] = {0, 0x50000000u,
                0x17000000u, 0x11000000u};
            // w - put ptr; prev (w) - newest; prev (c, w) - oldest
            int j = w - 3*_c;
            if (j < 0) j += 3*T;
            glPushMatrix (), glLoadIdentity ();
            // glBegin (GL_LINE_STRIP);
            for (int k = _c; k; k--, j += 3, j %= 3 * T) {
                rc.Point (&t[j], (color & 0xffffffu) | F[k]);
            //    unsigned int cc = (color & 0xffffffu) | F[k];
            //    glColor4ubv ((const GLubyte *)&cc), glVertex3fv (t+j);
            }
            // unsigned int cc = color;
            // glColor4ubv ((const GLubyte *)&cc), glVertex3fv (p);
            // glEnd ();
            glPopMatrix ();
            rc.Star (p, shape, shape_colors);
            // rc.Point (p, color);
            // unsigned int c2 = color & 0x15ffffffu;
            // glBegin (GL_LINES);
            //    glColor4ubv ((const GLubyte *)&c2), glVertex3fv (p);
            if (! paused) move ();
            //    glColor4ubv ((const GLubyte *)&color), glVertex3fv (p);
            // glEnd ();
        }
        void regen()
        {
            //TODO cmd line param
            const int C = 7; // en.wikipedia.org/wiki/Stellar_classification
            const unsigned int colors[C] = {0xff9db4ffu, 0xffffccbbu,
                0xfffff8fbu, 0xffedffffu, 0xff00ffffu, 0xff3398ffu,
                0xff0000ffu};
            // In order to simulate semi-space flight, the stars have to "flow"
            // alongside the frustum w/o hitting the near clip plane (stars
            // should disappear on screen edges only - not out of the blue in
            // the middle of the screen)
            p[0] = DELTA + rndf (FWIDTH/2.f, 100); if (rnd (2)) p[0] = -p[0];
            p[1] = DELTA + rndf (FHEIGHT/2.f, 100); if (rnd (2)) p[1] = -p[1];
            // p[2] = (-NEAR - (DEPTH*4.f/5.f)) - rndf (DEPTH*1.f/5.f, 100);
            p[2] = (-FNEAR - (FDEPTH*1.f/2.f)) - rndf (FDEPTH*1.f/2.f, 100);
            compute_shape();
            w = _c = 0;
            //D p[0] = 0.3f; p[1] = 0.6f; p[2] = -1.3f;
            color = colors[rnd (C)];
            shape_colors[0] = color, shape_colors[1] = color & 0x23ffffffu,
            shape_colors[2] = color & 0x84ffffffu;
            size = 1 + rnd (4);
            speed_rz = 0.5f + rndf (1.5f, 100);
            //D project from 0, 0; dx = p[0], dy = p[1]; y = ax + b
            if (rnd(2)) { // random delta_x
                float k = p[1] / p[0];
                _d[0] = 0.002f; // (1 + rnd (10)) / 1000.f; // delta_x
                if (p[0] < 0) _d[0] = -_d[0];
                //D d[0] = 0.1f;
                _d[1] = _d[0] * k; // delta_y
            } else { // random delta_y
                float k = p[0] / p[1];
                _d[1] = 0.002f; // (1 + rnd (10)) / 1000.f; // delta_y
                if (p[1] < 0) _d[1] = -_d[1];
                //D d[0] = 0.1f;
                _d[0] = _d[1] * k; // delta_x
            }
            // avoid complicated projection (x and y towards 1, z towards 0)
            // ensure delta_z == min (delta_x, delta_y)
            int sx = ROUND((1.f-ABS(p[0]))/ABS(_d[0])), // steps to reach 1 == x
                sy = ROUND((1.f-ABS(p[1]))/ABS(_d[1])); // steps to reach 1 == y
            _d[2] = (-p[2]-FNEAR) / (sx < sy ? sx : sy);
            //D printf ("p: %.2f %.2f %.2f, d: %.2f %.2f %.2f, s: %d %d %.1f\n",
                //D p[0], p[1], p[2], d[0], d[1], d[2], sx, sy,
                //D (-p[2]-NEAR)/d[2]);
        }
    } * stars {0};
    int star_count {0};

    Space()
    {
        srandom ((unsigned)time(NULL));
        int sc = 151 + rnd (150); //TODO this becomes cmd line param
        const int BYTES = sc * sizeof(Star);
        stars = (Star *)calloc (sc, sizeof(Star));
        if (!stars) {
            printf ("failed to allocate %d bytes\n", BYTES);
            return;
        }
        // the complicated "new" just calls the constructor (init. the fields)
        for (int i = 0; i < sc; (new (&(stars[i++])) Star())->regen ()) ;
        // move them 50 times prior rendering
        for (int j = 0; j < 50; j++)
            for (int i = 0; i < sc; stars[i++].move ()) ;
        star_count = sc;

        star_view = (node *)calloc (1, sizeof(node));
        if (! star_view) {
            printf ("failed to allocate %d bytes\n", sizeof(node));
            return;
        }
        star_view->star = stars;
        star_tail = star_view;
        for (int i = 1; i < star_count; i++) {
            node * n = (node *)calloc (1, sizeof(node));
            if (! n) {
                printf ("failed to allocate %d bytes\n", sizeof(node));
                return;
            }
            n->star = stars + i;
            star_tail->next = n, star_tail = n;
        }
    }
    ~Space()
    {
        if (stars) free (stars), stars = 0;
        node * tmp;
        while (star_view)
            tmp = star_view->next, free (star_view), star_view = tmp;
        star_view = star_tail = 0;
    }
    void Render()
    {
        Sort ();
        for (int i = 0; i < star_count; stars[i++].render (R)) ;
    }

    // Things always look better when sorted - for Open GL that's literally.
    // Since the renderer above is doing: render0 move0, render1 move1, ...,
    // the sorting shall be done prior the for loop, each time (the star
    // acceleration could become non-const at some point)
    void Sort() // slow and simple; but fast enough
    {//TODONT optimize
        for (int i = 1; i < star_count; i++)
            if (stars[i].p[2] < stars[i-1].p[2]) { // found something unsorted
                // quickly find its right place
                //D sort_moves++;
                int m = i-2, m2 = i-1;
                //D while (m >= 0 && stars[i].p[2] < stars[m].p[2]) m--;
                //D m++; // at [m] [i] is not <, so insert after it at [m+1]
                int a = 0, b = m2 < 0 ? 0 : m2;
                //D int bugcheck = 0;
                //D printf ("a: %d, b: %d\n", a, b);
                while (a != b) {
                    //D if (bugcheck++ > star_count) // log2(star_count)
                    //D    printf ("eternal loop\n"), exit (1);
                    int t = a + (b-a)/2;
                    //D printf (" a: %d, b: %d, t:%d\n", a, b, t);
                    if (stars[i].p[2] >= stars[t].p[2])
                        a = t + 1;
                    else b = t;
                } // binary search
                //D if (m != a) {
                //D     printf ("linear search %d != binary search %d\n", m, a);
                //D     exit (1);
                //D }
                m = a;

                // put it where it should be: delete (i), insert (m, i);
                Star tmp;
                memcpy (&tmp, stars + i, sizeof(Star)); // t = a[i]
                // make a place for tmp - shift right with 1
                memmove (stars + m+1, stars + m, (i-m) * sizeof(Star));
                memcpy (stars + m, &tmp, sizeof(Star)); // a[m] = t;
                //D sort_copied_bytes+=((i-m)+2)*sizeof(Star);
            } // if something unsorted
        //TODO to the test suite
        //D printf ("star_count:%d\n", star_count);
        //D for (int i = 1; i < star_count; i++)
        //D    if (stars[i].p[2] < stars[i-1].p[2])
        //D        printf ("bug, Sort failed: z[%d]:%.2f < z[%d]:%.2f\n",
        //D            i, stars[i].p[2], i-1, stars[i-1].p[2]);
    }
    struct node {
        Star * star;
        node * next;
    } * star_view {0}, * star_tail {0};
    // the same as the above sort, but using a linked list view of the data
    /* void sort_with_a_view() //TODO complete me; sorting a linked list will
     * simplify the above code
    {
        for (node * i = star_view; i && i->next; i = i->next)
            if (i->next->star->p[2] < i->star->p[2]) {
                node * next_i = i->next->next;
                for (node * k = star_view, * p = 0; k && k->next; p = k,
                    k = k->next)
                    if (k->star->p[2] > i->next->star->p[2])
                        ;// delete_insert ({i, i->next}, {p, k});
            }
    } */
} Space;

//D static GLfloat ry = 0.f;

static void glutDisplay()
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //D glLoadIdentity ();
    //D glTranslatef (0.f , 0.f , -(NEAR+DEPTH/2.f));
    //D glRotatef (ry, 0.f, 1.f, 0.f);
    //D glTranslatef (0.f , 0.f , +(NEAR+DEPTH/2.f));
    Space.Render ();
    //D float const ZN = NEAR + .01f, ZF = FAR - .01f;
    //D glColor3f(0, 1, 0);
    //D glBegin (GL_LINE_STRIP);
    //D     glVertex3f (1, 1, -ZN), glVertex3f (1, -1, -ZN),
    //D     glVertex3f (-1, -1, -ZN), glVertex3f (-1, 1, -ZN),
    //D         glVertex3f (1, 1, -ZN);
    //D glEnd ();
    //D glColor3f(1, 1, 1);
    //D glBegin (GL_LINE_STRIP);
    //D     glVertex3f (1, 1, -ZF), glVertex3f (1, -1, -ZF),
    //D     glVertex3f (-1, -1, -ZF), glVertex3f (-1, 1, -ZF),
    //D         glVertex3f (1, 1, -ZF);
    //D glEnd ();
    glutSwapBuffers ();
}

// auto-hide the mouse cursor on inactivity when full-screen
static time_t mouse_moved, tmouse;
static bool fs = false;

static void glutIdle()
{
    if (fs) {
        auto df = difftime (tmouse = time (0), mouse_moved);
        if (df > 3.0)
            glutSetCursor (GLUT_CURSOR_NONE);
    }
    usleep (40000), glutPostRedisplay ();
}

static void glutReshape(GLsizei width, GLsizei height)
{
    if (height < 1) height = 1;
    if (width < 1) width = 1;
    current_viewport_height = height;
    current_viewport_width = width;
    glViewport (0, 0, width, height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glFrustum (FLEFT, FRIGHT, FBOTTOM, FTOP, FNEAR, FFAR);
    glMatrixMode (GL_MODELVIEW);
    // glLoadIdentity ();
}

static GLvoid glutKeyPressed(unsigned char key, int, int)
{
    static int x = 0, y = 0, w = 320, h = 200;
    if ('p' == key) paused = ! paused;
    else if ('q' == key) exit (0);
    //D else if ('a' == key) ry += 1.f;
    //D else if ('d' == key) ry -= 1.f;
    else if (key == 'f') {
        if (!fs) {
            x = glutGet (GLUT_WINDOW_X), y = glutGet (GLUT_WINDOW_X);
            w = glutGet (GLUT_WINDOW_WIDTH), h = glutGet (GLUT_WINDOW_HEIGHT);
            glutFullScreen ();
            fs = true;
            mouse_moved = time (0);
        } else
            glutSetCursor (GLUT_CURSOR_INHERIT),
            glutReshapeWindow (w, h), glutPositionWindow (x, y), fs = false;
    }
}

static GLvoid glutPassiveMotion(int, int)
{
    mouse_moved = time (0);
    glutSetCursor (GLUT_CURSOR_INHERIT);
}

int main()
{
    int GLUT_ARGC = 0;
    glutInit (&GLUT_ARGC, 0);
    current_viewport_width = glutGet (GLUT_SCREEN_WIDTH);
    current_viewport_height = glutGet (GLUT_SCREEN_HEIGHT);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_ALPHA);
    glutInitWindowSize (current_viewport_width, current_viewport_height);
    glutInitWindowPosition (0, 0);
    glutCreateWindow ("space gl 1.0");
    glEnable (GL_COLOR_MATERIAL);
    glDisable (GL_LIGHTING);
    glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth (1.0f);
    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);
    glEnable (GL_POINT_SMOOTH);
    // glDisable (GL_ALPHA_TEST);
    // glAlphaFunc (GL_GEQUAL, 1.0f);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE); // GL_ONE_MINUS_SRC_ALPHA
    //D glPointSize (2);
    //D glLineWidth (2);
    glutKeyboardFunc (glutKeyPressed);
    glutReshapeFunc (glutReshape);
    glutDisplayFunc (glutDisplay);
    glutIdleFunc (glutIdle);
    glutPassiveMotionFunc (glutPassiveMotion);
    mouse_moved = time (0);
    glutMainLoop ();
    return 0;
}