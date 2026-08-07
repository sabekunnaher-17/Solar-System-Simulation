#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>



#define PI 3.14159265359f

float timeScale = 1.0f;
bool showHelp = true;


float simulationDays = 0.0f;
int simulationYears = 0;
int simulationDayOfYear = 0;


float cometT = 0.0f;
float cometSpeed = 0.03f;

float cometP0[3] = {-900, 0, -600};
float cometP1[3] = {-300, 0,  400};
float cometP2[3] = { 300, 0, -400};
float cometP3[3] = { 900, 0,  600};

struct Vec3
{
    float x, y, z;
};



// ================= DDA LINE ALGORITHM =================

void drawDDALine(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    float steps = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);
    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = x1;
    float y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++)
    {
        glVertex3f(x, 0.0f, y);
        x += xInc;
        y += yInc;
    }
    glEnd();
}

// ================= ORBIT USING DDA ====================

void drawOrbit(float r)
{
    glDisable(GL_LIGHTING);
    glColor3f(0.35f, 0.35f, 0.35f);

    for (int i = 0; i < 360; i++)
    {
        float a1 = i * PI / 180.0f;
        float a2 = (i + 1) * PI / 180.0f;

        drawDDALine(
            cos(a1) * r, sin(a1) * r,
            cos(a2) * r, sin(a2) * r
        );
    }

    glEnable(GL_LIGHTING);
}

// ================= MIDPOINT CIRCLE ===================

void midpointCircle(int r)
{
    int x = 0;
    int y = r;
    int d = 1 - r;

    glBegin(GL_POINTS);
    while (x <= y)
    {
        glVertex3f( x, 0,  y);
        glVertex3f( y, 0,  x);
        glVertex3f(-x, 0,  y);
        glVertex3f(-y, 0,  x);
        glVertex3f(-x, 0, -y);
        glVertex3f(-y, 0, -x);
        glVertex3f( x, 0, -y);
        glVertex3f( y, 0, -x);

        if (d < 0)
            d += 2 * x + 3;
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
    glEnd();
}

void midpointCircleAt(float cx, float cz, int r)
{
    int x = 0;
    int y = r;
    int d = 1 - r;
    glDisable(GL_LIGHTING);
    glPointSize(6.0f);
    glColor3f(1.0f, 0.5f, 0.2f);

    glBegin(GL_POINTS);
    int skip = 3;

    while (x <= y)
    {
        if (x % skip == 0)
        {
            glVertex3f( cx + x, 0, cz + y);
            glVertex3f( cx + y, 0, cz + x);
            glVertex3f( cx - x, 0, cz + y);
            glVertex3f( cx - y, 0, cz + x);

            glVertex3f( cx - x, 0, cz - y);
            glVertex3f( cx - y, 0, cz - x);
            glVertex3f( cx + x, 0, cz - y);
            glVertex3f( cx + y, 0, cz - x);
        }

        if (d < 0) d += 2 * x + 3;
        else { d += 2 * (x - y) + 5; y--; }

        x++;
    }


    glEnd();

    glPointSize(1.0f);
    glEnable(GL_LIGHTING);
}


// ================= BEZIER CURVE ======================

void drawBezier(float p0[], float p1[], float p2[], float p3[])
{
    glDisable(GL_LIGHTING);
    glColor3f(0.6f, 0.6f, 0.6f);

    glBegin(GL_LINE_STRIP);
    for (float t = 0.0f; t <= 1.0f; t += 0.01f)
    {
        float x =
            pow(1 - t, 3) * p0[0] +
            3 * t * pow(1 - t, 2) * p1[0] +
            3 * t * t * (1 - t) * p2[0] +
            pow(t, 3) * p3[0];

        float z =
            pow(1 - t, 3) * p0[2] +
            3 * t * pow(1 - t, 2) * p1[2] +
            3 * t * t * (1 - t) * p2[2] +
            pow(t, 3) * p3[2];

        glVertex3f(x, 0.0f, z);
    }
    glEnd();

    glEnable(GL_LIGHTING);
}



enum ViewMode {
    VIEW_GLOBAL,
    VIEW_EARTH,
    VIEW_MARS,
    VIEW_JUPITER,
    VIEW_SATURN,
    VIEW_URANUS,
    VIEW_NEPTUNE
};


ViewMode currentView = VIEW_GLOBAL;

// ======================STAR FIELD ====================

struct Star
{
    float x, y, z;
};

struct Asteroid
{
    float angle;
    float distance;
    float size;
};


std::vector<Star> stars;

void generateStars(int n)
{
    stars.clear();
    srand(1);

    for (int i = 0; i < n; i++)
    {
        stars.push_back({
            float(rand() % 6000 - 3000),
            float(rand() % 6000 - 3000),
            float(rand() % 6000 - 3000)
        });
    }
}

void drawStars()
{
    glDisable(GL_LIGHTING);
    glPointSize(2.0f);

    glBegin(GL_POINTS);
    for (auto &s : stars)
        glVertex3f(s.x, s.y, s.z);
    glEnd();

    glEnable(GL_LIGHTING);
}

// ===================LIGHT SETUP======================

void setupLight()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    float ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
    float diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    float position[] = {0.0f, 0.0f, 0.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, position);


    float globalAmb[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
}

// ===================GLOBAL CAMERA STATE===============

float globalZoom   = 1800.0f;
float globalHeight = 700.0f;


// =================BASE CLASS : CELESTIAL BODY=============

class CelestialBody
{
public:
    float radius;
    float selfSpeed;
    float selfAngle;
    float axialTilt;



    CelestialBody* parent;
    float orbitRadius;
    float orbitSpeed;
    float orbitAngle;

    bool ignoreParent = false;
    bool visibleInGlobal = true;

    CelestialBody(float r)
    : radius(r), selfSpeed(0), selfAngle(0),
      parent(nullptr), orbitRadius(0),
      orbitSpeed(0), orbitAngle(0),
      axialTilt(0.0f) {}


    virtual ~CelestialBody() {}

    void setSelfRotationSpeed(float speed)
    {
        selfSpeed = speed;
    }

    void setOrbit(CelestialBody* p, float r, float speed)
    {
        parent = p;
        orbitRadius = r;
        orbitSpeed = speed;
    }

    void setIgnoreParent(bool v)
    {
        ignoreParent = v;
    }

    void setVisibleInGlobal(bool v)
    {
        visibleInGlobal = v;
    }

    bool isVisibleInGlobal() const
    {
        return visibleInGlobal && !ignoreParent;
    }

    virtual void update(float dt)
    {
        selfAngle  += selfSpeed  * dt;
        orbitAngle += orbitSpeed * dt;
    }

    void applyTransform()
    {
        if (parent && !ignoreParent)
        {
            parent->applyTransform();
            glRotatef(orbitAngle, 0, 1, 0);
            glTranslatef(orbitRadius, 0, 0);
        }
        glRotatef(axialTilt, 0, 0, 1);

        glRotatef(selfAngle, 0, 1, 0);
    }

    virtual void draw() = 0;

    void setAxialTilt(float tilt)
{
    axialTilt = tilt;
}

};

// ======================SUN ========================

class Sun : public CelestialBody
{
public:
    Sun() : CelestialBody(40)
    {
        selfSpeed = 6;
    }
    void draw() override
    {
        glPushMatrix();
        applyTransform();


        float sunAmbDiff[] = {1.0f, 0.9f, 0.4f, 1.0f};
        float sunEmission[] = {0.4f, 0.3f, 0.1f, 1.0f};
        float sunSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sunAmbDiff);
        glMaterialfv(GL_FRONT, GL_EMISSION, sunEmission);
        glMaterialfv(GL_FRONT, GL_SPECULAR, sunSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 30.0f);

        glutSolidSphere(radius, 60, 60);


        float zero[] = {0.0f, 0.0f, 0.0f, 1.0f};
        float defaultGray[] = {0.8f, 0.8f, 0.8f, 1.0f};

        glMaterialfv(GL_FRONT, GL_EMISSION, zero);
        glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, defaultGray);

        glPopMatrix();
    }
};


class Planet;
Planet* perspectivePlanet = nullptr;

// ===================PLANET =======================

class Planet : public CelestialBody
{
protected:
    float base[3];
    float day[3];
    float night[3];
    std::vector<Vec3> trail;


public:
    Planet(float r) : CelestialBody(r) {}

    void update(float dt) override
    {
        CelestialBody::update(dt);

        float x = orbitRadius * cosf(orbitAngle * PI / 180.0f);
        float z = orbitRadius * sinf(orbitAngle * PI / 180.0f);

        trail.push_back({x, 0.0f, z});

        if (trail.size() > 200)
            trail.erase(trail.begin());
    }

    void setColors(float b[3], float d[3], float n[3])
    {
        memcpy(base,  b, 3 * sizeof(float));
        memcpy(day,   d, 3 * sizeof(float));
        memcpy(night, n, 3 * sizeof(float));
    }

    void draw() override
    {
        glPushMatrix();

        if (currentView == VIEW_GLOBAL)
    drawTrail();

        if (ignoreParent)
            glRotatef(selfAngle, 0, 1, 0);
        else
            applyTransform();



        if (currentView == VIEW_GLOBAL && perspectivePlanet == this)
        {

        }
        else
        {
            if (currentView == VIEW_GLOBAL)
            {
                if (visibleInGlobal)
                    drawDayNightSphere();
            }
            else
            {
                drawDayNightSphere();
            }
        }

        glPopMatrix();
    }

protected:

    void drawTrail()
    {
        glDisable(GL_LIGHTING);
        glColor3f(0.6f, 0.6f, 0.6f);

        glBegin(GL_LINE_STRIP);
        for (auto &p : trail)
            glVertex3f(p.x, p.y, p.z);
        glEnd();

        glEnable(GL_LIGHTING);
    }
    void drawDayNightSphere()
    {

        float noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};
        float blackSpec[] = {0.0f, 0.0f, 0.0f, 1.0f};
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
        glMaterialfv(GL_FRONT, GL_SPECULAR, blackSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);

        glBegin(GL_QUAD_STRIP);
        for (int lat = -90; lat <= 90; lat += 5)
        {
            float lat0 = lat * PI / 180.0f;
            float lat1 = (lat + 5) * PI / 180.0f;
            for (int lon = 0; lon <= 360; lon += 10)
            {
                float l = lon * PI / 180.0f;
                float* c = (cos(l) > 0) ? day : night;
                float mat[4] = { c[0], c[1], c[2], 1.0f };
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat);

                glNormal3f(cos(lat0)*cos(l), sin(lat0), cos(lat0)*sin(l));
                glVertex3f(radius*cos(lat0)*cos(l),
                           radius*sin(lat0),
                           radius*cos(lat0)*sin(l));

                glNormal3f(cos(lat1)*cos(l), sin(lat1), cos(lat1)*sin(l));
                glVertex3f(radius*cos(lat1)*cos(l),
                           radius*sin(lat1),
                           radius*cos(lat1)*sin(l));
            }
        }
        glEnd();
    }

};

// =================== MOON ========================

class Moon : public Planet
{
public:
    Moon(float r) : Planet(r)
    {
        selfSpeed = 40;
    }

    void draw() override
    {
        glPushMatrix();

        glRotatef(orbitAngle, 0, 1, 0);
        glTranslatef(orbitRadius, 0, 0);
        glRotatef(selfAngle, 0, 1, 0);

        if (currentView == VIEW_GLOBAL)
        {
            if (visibleInGlobal)
                drawDayNightSphere();
        }
        else
        {
            drawDayNightSphere();
        }

        glPopMatrix();
    }
};

// ==================SATURN RING SYSTEM =================

class RingSystem : public CelestialBody
{
    float innerRadius;
    float outerRadius;

public:
    RingSystem(float inner, float outer)
        : CelestialBody(0), innerRadius(inner), outerRadius(outer) {}

    void draw() override
    {
        glPushMatrix();
        applyTransform();

        if (currentView == VIEW_GLOBAL && !isVisibleInGlobal())
        {
            glPopMatrix();
            return;
        }

        glRotatef(26.7f, 1.0f, 0.0f, 0.0f);

        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const int segments = 360;
        const int layers = 100;

        for (int i = 0; i < layers; i++)
        {
            float t = (float)i / layers;
            float r1 = innerRadius + t * (outerRadius - innerRadius);
            float r2 = innerRadius + (t + 1.0f / layers) * (outerRadius - innerRadius);

            float alpha = 0.85f * (1.0f - t * 0.5f);
            glColor4f(0.85f - t * 0.3f,
                      0.75f - t * 0.3f,
                      0.60f - t * 0.3f,
                      alpha);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= segments; j++)
            {
                float ang = j * 2 * PI / segments;
                float x = cosf(ang);
                float z = sinf(ang);

                glVertex3f(x * r1, 0.01f, z * r1);
                glVertex3f(x * r2, 0.01f, z * r2);
            }
            glEnd();
        }

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
};

class MagneticField : public CelestialBody
{
    float fieldRadius;

public:
    MagneticField(float r) : CelestialBody(0), fieldRadius(r) {}

    void draw() override
    {
        if (currentView != VIEW_GLOBAL) return;

        glPushMatrix();
        applyTransform();

        glDisable(GL_LIGHTING);
        glPointSize(2.0f);
        glColor3f(0.3f, 0.8f, 1.0f);

        glBegin(GL_POINTS);
        for (int i = 0; i < 360; i += 4)
        {
            float a = i * PI / 180.0f;
            glVertex3f(
                cos(a) * fieldRadius,
                0.0f,
                sin(a) * fieldRadius
            );
        }
        glEnd();

        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
};

class AsteroidBelt : public CelestialBody
{
    std::vector<Asteroid> asteroids;
    float innerRadius = 300.0f;
    float outerRadius = 380.0f;

public:
    AsteroidBelt(int count) : CelestialBody(0)
{
    asteroids.clear();
    srand(42);
    for (int i = 0; i < count; i++)
    {
        Asteroid a;
        a.angle = (float)(rand() % 3600) / 10.0f;
        a.distance = innerRadius + (float)(rand() % (int)((outerRadius - innerRadius) * 10)) / 10.0f;
        a.size = 1.2f + (float)(rand() % 30) / 30.0f;
        asteroids.push_back(a);
    }
}

    void update(float dt) override
    {
        for (auto& a : asteroids)
        {
            a.angle += 12.0f * dt;
        }
    }

    void draw() override
    {
        if (currentView != VIEW_GLOBAL) return;

        glPushMatrix();
        applyTransform();




        glEnable(GL_LIGHTING);
        for (const auto& a : asteroids)
        {
            float rad = a.angle * PI / 180.0f;
            float x = cosf(rad) * a.distance;
            float z = sinf(rad) * a.distance;

            glPushMatrix();
            glTranslatef(x, 0.0f, z);

            float matAmb[]  = {0.7f, 0.7f, 0.7f, 1.0f};
            float matDiff[] = {0.9f, 0.9f, 0.8f, 1.0f};
            glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiff);

            glutSolidSphere(a.size, 10, 10);

            glPopMatrix();
        }

        glPopMatrix();
    }
};

// ===================SOLAR SYSTEM CONTAINER============

class SolarSystem
{
    std::vector<CelestialBody*> bodies;

public:
    ~SolarSystem()
    {
        for (auto b : bodies)
            delete b;
    }

    void add(CelestialBody* b)
    {
        bodies.push_back(b);
    }

    void update(float dt)
    {
        for (auto b : bodies)
            b->update(dt);
    }

    void draw()
    {
        for (auto b : bodies)
            b->draw();
    }
};

SolarSystem solarSystem;

// ================GLOBAL OBJECT POINTERS==================

Sun* sun;

Planet* mercury;
Planet* venus;
Planet* earth;
Planet* mars;
Planet* jupiter;
Planet* saturn;
Planet* uranus;
Planet* neptune;

Moon* moon;
Moon* phobos;
Moon* deimos;

Moon* io;
Moon* europa;
Moon* ganymede;
Moon* callisto;

Moon* titan;
Moon* rhea;
Moon* iapetus;
Moon* dione;
Moon* tethys;
Moon* enceladus;
Moon* mimas;

Moon* titania;
Moon* oberon;
Moon* umbriel;
Moon* ariel;
Moon* miranda;

Moon* triton;
Moon* proteus;
Moon* nereid;

RingSystem* saturnRings;
AsteroidBelt* asteroidBelt;
MagneticField* earthMagField;
MagneticField* jupiterMagField;



// =================INIT SOLAR SYSTEM ================

void initSolarSystem()
{

    srand(time(0));

    // SUN
    sun = new Sun();
    solarSystem.add(sun);

    // MERCURY
    mercury = new Planet(8);
    mercury->setOrbit(sun, 120, 22);
    mercury->setSelfRotationSpeed(10);
    float mb[] = {0.55f, 0.50f, 0.45f};
    float md[] = {0.70f, 0.65f, 0.60f};
    float mn[] = {0.25f, 0.20f, 0.15f};
    mercury->setColors(mb, md, mn);
    solarSystem.add(mercury);

    // VENUS
    venus = new Planet(14);
    venus->setOrbit(sun, 170, 15);
    venus->setSelfRotationSpeed(6);
    float vb[] = {0.90f, 0.80f, 0.55f};
    float vd[] = {1.00f, 0.90f, 0.70f};
    float vn[] = {0.50f, 0.40f, 0.25f};
    venus->setColors(vb, vd, vn);
    solarSystem.add(venus);

    // EARTH
    earth = new Planet(16);
    earth->setOrbit(sun, 220, 12);
    earth->setSelfRotationSpeed(60);
    float eb[] = {0.10f, 0.30f, 0.70f};
    float ed[] = {0.20f, 0.60f, 1.00f};
    float en[] = {0.02f, 0.05f, 0.20f};
    earth->setColors(eb, ed, en);
    solarSystem.add(earth);

    earthMagField = new MagneticField(28);
    earthMagField->setOrbit(earth, 0, 0);
    solarSystem.add(earthMagField);


    // Moon
    moon = new Moon(6);
    moon->setOrbit(earth, 55, 70);
    moon->setVisibleInGlobal(false);
    float moonb[] = {0.60f, 0.60f, 0.60f};
    float moond[] = {0.80f, 0.80f, 0.80f};
    float moonn[] = {0.30f, 0.30f, 0.30f};
    moon->setColors(moonb, moond, moonn);
    solarSystem.add(moon);

    // MARS
    mars = new Planet(12);
    mars->setOrbit(sun, 280, 9);
    mars->setSelfRotationSpeed(50);
    float marsb[] = {0.85f, 0.45f, 0.30f};
    float marsd[] = {0.95f, 0.60f, 0.40f};
    float marsn[] = {0.40f, 0.20f, 0.10f};
    mars->setColors(marsb, marsd, marsn);
    solarSystem.add(mars);

    // Mars moons
    phobos = new Moon(4);
    phobos->setOrbit(mars, 28, 90);
    phobos->setVisibleInGlobal(false);
    float phb[] = {0.55f, 0.50f, 0.45f};
    float phd[] = {0.65f, 0.60f, 0.55f};
    float phn[] = {0.30f, 0.25f, 0.20f};
    phobos->setColors(phb, phd, phn);
    solarSystem.add(phobos);

    deimos = new Moon(3);
    deimos->setOrbit(mars, 40, 45);
    deimos->setVisibleInGlobal(false);
    deimos->setColors(phb, phd, phn);
    solarSystem.add(deimos);

    // JUPITER
    jupiter = new Planet(35);
    jupiter->setOrbit(sun, 420, 5);
    jupiter->setSelfRotationSpeed(120);
    float jb[] = {0.85f, 0.70f, 0.50f};
    float jd[] = {0.95f, 0.85f, 0.65f};
    float jn[] = {0.50f, 0.35f, 0.20f};
    jupiter->setColors(jb, jd, jn);
    solarSystem.add(jupiter);

    jupiterMagField = new MagneticField(55);
    jupiterMagField->setOrbit(jupiter, 0, 0);
    solarSystem.add(jupiterMagField);



    // Jupiter moons color
    float jmoonb[] = {0.65f, 0.65f, 0.60f};
    float jmoond[] = {0.80f, 0.80f, 0.75f};
    float jmoonn[] = {0.40f, 0.40f, 0.35f};

    io = new Moon(7);
    io->setOrbit(jupiter, 45, 85);
    io->setVisibleInGlobal(false);
    io->setColors(jmoonb, jmoond, jmoonn);
    solarSystem.add(io);

    europa = new Moon(6);
    europa->setOrbit(jupiter, 60, 65);
    europa->setVisibleInGlobal(false);
    europa->setColors(jmoonb, jmoond, jmoonn);
    solarSystem.add(europa);

    ganymede = new Moon(9);
    ganymede->setOrbit(jupiter, 80, 45);
    ganymede->setVisibleInGlobal(false);
    ganymede->setColors(jmoonb, jmoond, jmoonn);
    solarSystem.add(ganymede);

    callisto = new Moon(8);
    callisto->setOrbit(jupiter, 105, 30);
    callisto->setVisibleInGlobal(false);
    callisto->setColors(jmoonb, jmoond, jmoonn);
    solarSystem.add(callisto);

    // ASTEROID BELT
    asteroidBelt = new AsteroidBelt(600);
    asteroidBelt->setOrbit(sun, 0, 0);
    asteroidBelt->setVisibleInGlobal(true);
    solarSystem.add(asteroidBelt);


    // SATURN
    saturn = new Planet(30);
    saturn->setOrbit(sun, 550, 3);
    saturn->setSelfRotationSpeed(110);
    float sb[] = {0.90f, 0.80f, 0.60f};
    float sd[] = {1.00f, 0.90f, 0.75f};
    float sn[] = {0.50f, 0.40f, 0.25f};
    saturn->setColors(sb, sd, sn);
    solarSystem.add(saturn);

    saturnRings = new RingSystem(40, 100);
    saturnRings->setOrbit(saturn, 0, 0);
    solarSystem.add(saturnRings);

    // Saturn moons color
    float smoonb[] = {0.60f, 0.60f, 0.55f};
    float smoond[] = {0.75f, 0.75f, 0.70f};
    float smoonn[] = {0.35f, 0.35f, 0.30f};

    titan = new Moon(10);
    titan->setOrbit(saturn, 120, 25);
    titan->setVisibleInGlobal(false);
    titan->setColors(smoonb, smoond, smoonn);
    solarSystem.add(titan);

    rhea = new Moon(8);
    rhea->setOrbit(saturn, 80, 35);
    rhea->setVisibleInGlobal(false);
    rhea->setColors(smoonb, smoond, smoonn);
    solarSystem.add(rhea);

    iapetus = new Moon(7);
    iapetus->setOrbit(saturn, 180, 15);
    iapetus->setVisibleInGlobal(false);
    iapetus->setColors(smoonb, smoond, smoonn);
    solarSystem.add(iapetus);

    dione = new Moon(6);
    dione->setOrbit(saturn, 60, 45);
    dione->setVisibleInGlobal(false);
    dione->setColors(smoonb, smoond, smoonn);
    solarSystem.add(dione);

    tethys = new Moon(6);
    tethys->setOrbit(saturn, 50, 50);
    tethys->setVisibleInGlobal(false);
    tethys->setColors(smoonb, smoond, smoonn);
    solarSystem.add(tethys);

    enceladus = new Moon(5);
    enceladus->setOrbit(saturn, 40, 60);
    enceladus->setVisibleInGlobal(false);
    enceladus->setColors(smoonb, smoond, smoonn);
    solarSystem.add(enceladus);

    mimas = new Moon(4);
    mimas->setOrbit(saturn, 30, 70);
    mimas->setVisibleInGlobal(false);
    mimas->setColors(smoonb, smoond, smoonn);
    solarSystem.add(mimas);

    // URANUS
    uranus = new Planet(22);
    uranus->setOrbit(sun, 700, 2);
    uranus->setSelfRotationSpeed(80);
    float ub[] = {0.50f, 0.80f, 0.90f};
    float ud[] = {0.60f, 0.90f, 1.00f};
    float un[] = {0.20f, 0.40f, 0.50f};
    uranus->setColors(ub, ud, un);
    solarSystem.add(uranus);

    titania = new Moon(8);
    titania->setOrbit(uranus, 80, 30);
    titania->setVisibleInGlobal(false);
    titania->setColors(smoonb, smoond, smoonn);
    solarSystem.add(titania);

    oberon = new Moon(7);
    oberon->setOrbit(uranus, 100, 20);
    oberon->setVisibleInGlobal(false);
    oberon->setColors(smoonb, smoond, smoonn);
    solarSystem.add(oberon);

    umbriel = new Moon(6);
    umbriel->setOrbit(uranus, 60, 40);
    umbriel->setVisibleInGlobal(false);
    umbriel->setColors(smoonb, smoond, smoonn);
    solarSystem.add(umbriel);

    ariel = new Moon(6);
    ariel->setOrbit(uranus, 45, 55);
    ariel->setVisibleInGlobal(false);
    ariel->setColors(smoonb, smoond, smoonn);
    solarSystem.add(ariel);

    miranda = new Moon(5);
    miranda->setOrbit(uranus, 35, 70);
    miranda->setVisibleInGlobal(false);
    miranda->setColors(smoonb, smoond, smoonn);
    solarSystem.add(miranda);

    // NEPTUNE
    neptune = new Planet(21);
    neptune->setOrbit(sun, 850, 1.5f);
    neptune->setSelfRotationSpeed(90);
    float nb[] = {0.10f, 0.30f, 0.80f};
    float nd[] = {0.20f, 0.50f, 0.95f};
    float nn[] = {0.05f, 0.15f, 0.40f};
    neptune->setColors(nb, nd, nn);
    solarSystem.add(neptune);

    triton = new Moon(9);
    triton->setOrbit(neptune, 60, 45);
    triton->setVisibleInGlobal(false);
    triton->setColors(smoonb, smoond, smoonn);
    solarSystem.add(triton);

    proteus = new Moon(7);
    proteus->setOrbit(neptune, 40, 60);
    proteus->setVisibleInGlobal(false);
    proteus->setColors(smoonb, smoond, smoonn);
    solarSystem.add(proteus);

    nereid = new Moon(6);
    nereid->setOrbit(neptune, 120, 20);
    nereid->setVisibleInGlobal(false);
    nereid->setColors(smoonb, smoond, smoonn);
    solarSystem.add(nereid);

    earth->setAxialTilt(23.5f);
    mars->setAxialTilt(25.0f);
    jupiter->setAxialTilt(3.1f);
    saturn->setAxialTilt(26.7f);
    uranus->setAxialTilt(98.0f);
    neptune->setAxialTilt(28.3f);

}

// ==============CAMERA SETUP ===================

void setupCamera()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (currentView == VIEW_GLOBAL)
    {
        if (perspectivePlanet == nullptr)
        {

            gluLookAt(
                0, globalHeight, globalZoom,
                0, 0, 0,
                0, 1, 0
            );
        }
        else
        {

            float planetX = perspectivePlanet->orbitRadius * cosf(perspectivePlanet->orbitAngle * PI / 180.0f);
            float planetZ = perspectivePlanet->orbitRadius * sinf(perspectivePlanet->orbitAngle * PI / 180.0f);


            float camDistance = 400.0f;
            float camHeight = 250.0f;

            float camX = planetX - camDistance * cosf(perspectivePlanet->orbitAngle * PI / 180.0f);
            float camZ = planetZ - camDistance * sinf(perspectivePlanet->orbitAngle * PI / 180.0f);


            gluLookAt(
                camX, camHeight, camZ,
                0.0f, 50.0f, 0.0f,
                0.0f, 1.0f, 0.0f
            );
        }
    }
    else
    {

        if (currentView == VIEW_EARTH || currentView == VIEW_MARS)
            gluLookAt(0, 120, 90, 0, 0, 0, 0, 1, 0);
        else if (currentView == VIEW_JUPITER)
            gluLookAt(0, 200, 300, 0, 0, 0, 0, 1, 0);
        else if (currentView == VIEW_SATURN)
            gluLookAt(0, 250, 400, 0, 0, 0, 0, 1, 0);
        else if (currentView == VIEW_URANUS)
            gluLookAt(0, 200, 350, 0, 0, 0, 0, 1, 0);
        else if (currentView == VIEW_NEPTUNE)
            gluLookAt(0, 220, 380, 0, 0, 0, 0, 1, 0);
    }
}

// ============RESET TO GLOBAL VIEW=====================


void resetToGlobalView()
{
    currentView = VIEW_GLOBAL;
    perspectivePlanet = nullptr;

    earth->setIgnoreParent(false);
    mars->setIgnoreParent(false);
    if (jupiter) jupiter->setIgnoreParent(false);
    if (saturn)  saturn->setIgnoreParent(false);
    if (uranus)  uranus->setIgnoreParent(false);
    if (neptune) neptune->setIgnoreParent(false);
}

// ==============KEYBOARD INPUT ===================

void keyboard(unsigned char k, int, int)
{
    switch (k)
    {
    case 'b':
        resetToGlobalView();
        mercury->setSelfRotationSpeed(10);
        venus->setSelfRotationSpeed(6);
        earth->setSelfRotationSpeed(60);
        mars->setSelfRotationSpeed(50);
        jupiter->setSelfRotationSpeed(120);
        saturn->setSelfRotationSpeed(110);
        uranus->setSelfRotationSpeed(80);
        neptune->setSelfRotationSpeed(90);
        break;

    case 'e':
        currentView = VIEW_EARTH;
        earth->setIgnoreParent(true);
        break;
    case 'm':
        currentView = VIEW_MARS;
        mars->setIgnoreParent(true);
        break;
    case 'j':
        currentView = VIEW_JUPITER;
        jupiter->setIgnoreParent(true);
        break;
    case 's':
        currentView = VIEW_SATURN;
        saturn->setIgnoreParent(true);
        break;
    case 'u':
        currentView = VIEW_URANUS;
        uranus->setIgnoreParent(true);
        break;
    case 'n':
        currentView = VIEW_NEPTUNE;
        neptune->setIgnoreParent(true);
        break;

    case 'z':
    case 'Z':
        if (currentView == VIEW_GLOBAL)
        {
            globalZoom -= 150.0f;
            globalHeight -= 70.0f;
        }
        break;
    case 'x':
    case 'X':
        if (currentView == VIEW_GLOBAL)
        {
            globalZoom += 150.0f;
            globalHeight += 70.0f;
        }
        break;


    case '0':
        perspectivePlanet = nullptr;
        currentView = VIEW_GLOBAL;
        mercury->setSelfRotationSpeed(10);
        venus->setSelfRotationSpeed(6);
        earth->setSelfRotationSpeed(60);
        mars->setSelfRotationSpeed(50);
        jupiter->setSelfRotationSpeed(120);
        saturn->setSelfRotationSpeed(110);
        uranus->setSelfRotationSpeed(80);
        neptune->setSelfRotationSpeed(90);
        break;

    case '1':
        perspectivePlanet = mercury;
        mercury->setSelfRotationSpeed(5);
        break;
    case '2':
        perspectivePlanet = venus;
        venus->setSelfRotationSpeed(3);
        break;
    case '3':
        perspectivePlanet = earth;
        earth->setSelfRotationSpeed(20);
        break;
    case '4':
        perspectivePlanet = mars;
        mars->setSelfRotationSpeed(15);
        break;
    case '5':
        perspectivePlanet = jupiter;
        jupiter->setSelfRotationSpeed(40);
        break;
    case '6':
        perspectivePlanet = saturn;
        saturn->setSelfRotationSpeed(35);
        break;
    case '7':
        perspectivePlanet = uranus;
        uranus->setSelfRotationSpeed(20);
        break;
    case '8':
        perspectivePlanet = neptune;
        neptune->setSelfRotationSpeed(25);
        break;

    case 27:
        exit(0);
        break;

    case '-':
    timeScale *= 0.7f;
    if (timeScale < 0.05f) timeScale = 0.05f;
    break;

    case '+':
    timeScale *= 1.5f;
    if (timeScale > 10.0f) timeScale = 10.0f;
    break;

    case 'r':
    timeScale = 1.0f;
    break;

    case 'h':
    case 'H':
    showHelp = !showHelp;
    break;



    }
}


void drawComet()
{
    float t = cometT;

    float x =
        pow(1 - t, 3) * cometP0[0] +
        3 * t * pow(1 - t, 2) * cometP1[0] +
        3 * t * t * (1 - t) * cometP2[0] +
        pow(t, 3) * cometP3[0];

    float z =
        pow(1 - t, 3) * cometP0[2] +
        3 * t * pow(1 - t, 2) * cometP1[2] +
        3 * t * t * (1 - t) * cometP2[2] +
        pow(t, 3) * cometP3[2];

    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glutSolidSphere(6, 16, 16);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}



void drawText(float x, float y, const char* text)
{
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

void drawHelpMenu()
{
    float baseX = 950;
    float baseY = 760;

    drawText(baseX, baseY, "=== KEY GUIDE ===");

    drawText(baseX, baseY - 30, "Views:");
    drawText(baseX, baseY - 55, "e  - Earth View");
    drawText(baseX, baseY - 80, "m  - Mars View");
    drawText(baseX, baseY -105, "j  - Jupiter View");
    drawText(baseX, baseY -130, "s  - Saturn View");
    drawText(baseX, baseY -155, "u  - Uranus View");
    drawText(baseX, baseY -180, "n  - Neptune View");

    drawText(baseX, baseY -210, "Perspective:");
    drawText(baseX, baseY -235, "1-8  - Planet Perspective");
    drawText(baseX, baseY -260, "0    - Exit Perspective");

    drawText(baseX, baseY -290, "Time Control:");
    drawText(baseX, baseY -315, "+  - Fast Forward");
    drawText(baseX, baseY -340, "-  - Slow Motion");
    drawText(baseX, baseY -365, "r  - Reset Time");

    drawText(baseX, baseY -395, "Camera:");
    drawText(baseX, baseY -420, "z  - Zoom In");
    drawText(baseX, baseY -445, "x  - Zoom Out");

    drawText(baseX, baseY -475, "b  - Back to Global View");
    drawText(baseX, baseY -500, "ESC - Exit");
}




// ===============DISPLAY FUNCTION====================

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();
    drawStars();


    if (currentView == VIEW_GLOBAL)
    {
        drawOrbit(120);
        drawOrbit(170);
        drawOrbit(220);
        drawOrbit(280);
        drawOrbit(420);
        drawOrbit(550);
        drawOrbit(700);
        drawOrbit(850);

        solarSystem.draw();


    drawBezier(cometP0, cometP1, cometP2, cometP3);
    drawComet();
    }
    else if (currentView == VIEW_EARTH)
    {
        earth->draw();
        drawOrbit(55);
        moon->draw();
    }
    else if (currentView == VIEW_MARS)
    {
        mars->draw();
        drawOrbit(28);
        drawOrbit(40);
        phobos->draw();
        deimos->draw();
    }
    else if (currentView == VIEW_JUPITER)
    {
        jupiter->draw();
        drawOrbit(45);
        drawOrbit(60);
        drawOrbit(80);
        drawOrbit(105);
        io->draw();
        europa->draw();
        ganymede->draw();
        callisto->draw();
    }
    else if (currentView == VIEW_SATURN)
    {
        saturn->draw();
        drawOrbit(30);
        drawOrbit(40);
        drawOrbit(50);
        drawOrbit(60);
        drawOrbit(80);
        drawOrbit(120);
        drawOrbit(180);

        mimas->draw();
        enceladus->draw();
        tethys->draw();
        dione->draw();
        rhea->draw();
        titan->draw();
        iapetus->draw();
    }
    else if (currentView == VIEW_URANUS)
    {
        uranus->draw();
        drawOrbit(35);
        drawOrbit(45);
        drawOrbit(60);
        drawOrbit(80);
        drawOrbit(100);

        miranda->draw();
        ariel->draw();
        umbriel->draw();
        titania->draw();
        oberon->draw();
    }
    else if (currentView == VIEW_NEPTUNE)
    {
        neptune->draw();
        drawOrbit(40);
        drawOrbit(60);
        drawOrbit(120);

        proteus->draw();
        triton->draw();
        nereid->draw();
    }


    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1200, 0, 800);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();


    char clockText[100];
    sprintf(clockText,
            "Simulation Time: %d Years %d Days",
            simulationYears,
            simulationDayOfYear);

    drawText(20, 650, clockText);



    if (currentView == VIEW_EARTH)
    {
        drawText(20, 760, "Planet: Earth");
        drawText(20, 735, "Distance from Sun: 220");
        drawText(20, 710, "Rotation Speed: 60");
        drawText(20, 685, "Orbit Speed: 12");
    }
    else if (currentView == VIEW_MARS)
    {
        drawText(20, 760, "Planet: Mars");
        drawText(20, 735, "Distance from Sun: 280");
        drawText(20, 710, "Rotation Speed: 50");
        drawText(20, 685, "Orbit Speed: 9");
    }
    else if (currentView == VIEW_JUPITER)
    {
        drawText(20, 760, "Planet: Jupiter");
        drawText(20, 735, "Distance from Sun: 420");
        drawText(20, 710, "Rotation Speed: 120");
        drawText(20, 685, "Orbit Speed: 5");
    }
    else if (currentView == VIEW_SATURN)
    {
        drawText(20, 760, "Planet: Saturn");
        drawText(20, 735, "Distance from Sun: 550");
        drawText(20, 710, "Rotation Speed: 110");
        drawText(20, 685, "Orbit Speed: 3");
    }
    else if (currentView == VIEW_URANUS)
    {
        drawText(20, 760, "Planet: Uranus");
        drawText(20, 735, "Distance from Sun: 700");
        drawText(20, 710, "Rotation Speed: 80");
        drawText(20, 685, "Orbit Speed: 2");
    }
    else if (currentView == VIEW_NEPTUNE)
    {
        drawText(20, 760, "Planet: Neptune");
        drawText(20, 735, "Distance from Sun: 850");
        drawText(20, 710, "Rotation Speed: 90");
        drawText(20, 685, "Orbit Speed: 1.5");
    }

    if (showHelp)
    drawHelpMenu();


    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);


    glutSwapBuffers();
}


// ===============UPDATE FUNCTION===================

float lastTime = 0.0f;

void update()
{
    float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt = t - lastTime;
    lastTime = t;

    solarSystem.update(dt * timeScale);


    simulationDays += dt * timeScale * 10.0f;


    simulationYears = (int)(simulationDays / 365.0f);
    simulationDayOfYear = (int)simulationDays % 365;

    cometT += cometSpeed * dt * timeScale;
if (cometT > 1.0f)
    cometT = 0.0f;


    glutPostRedisplay();
}

// ==============INIT FUNCTION=====================

void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glClearColor(0, 0, 0, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1200.0 / 800.0, 1, 6000);

    setupLight();
    generateStars(2500);
    initSolarSystem();
}

// ===============MAIN FUNCTION======================

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("Solar System Model - Core");

    init();

    glutDisplayFunc(display);
    glutIdleFunc(update);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}

