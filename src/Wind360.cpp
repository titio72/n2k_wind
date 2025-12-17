#include <math.h>
#include "WindUtil.h"
#include "Wind360.h"

int angle_score(double a)
{
    if (a>=270.0) a -= 270.0;
    else if (a>=180.0) a -= 180.0;
    else if (a>=90.0) a -= 90.0;

    return (a > 15.0 || a < 75.0) ? 1 : 3;
}

Wind360::Wind360(int s): tot(0), n_samples(s)
{
    score = 0.0;
    tot_score = 0.0;
    if (n_samples==0)
    {
        sample_size = 0;
        data = nullptr;
        scores = nullptr;
    }
    else
    {
        sample_size = 360.0 / n_samples;
        data = new uint8_t[n_samples];
        scores = new uint8_t[n_samples];
        for (int i = 0; i<n_samples; i++)
        {
            tot_score += angle_score(i * sample_size);
        }
    }
    reset();
}

Wind360::Wind360(const Wind360 &w)
{
    tot = w.tot;
    n_samples = w.n_samples;
    sample_size = w.sample_size;
    score = w.score;
    tot_score = w.tot_score;

    if (n_samples==0)
    {
        data = nullptr;
        scores = nullptr;
    }
    else
    {
        data = new uint8_t[n_samples];
        scores = new uint8_t[n_samples];
        memcpy(data, w.data, n_samples * sizeof(uint8_t));
        memcpy(scores, w.scores, n_samples * sizeof(uint8_t));
    }
}

Wind360::~Wind360()
{
    if (data) delete[] data;
    if (scores) delete[] scores;
}

void Wind360::reset()
{
    if (data) memset(data, 0, n_samples * sizeof(uint8_t));
    if (scores) memset(scores, 0, n_samples * sizeof(uint8_t));
    score = 0.0;
    tot = 0;
}

bool Wind360::set_degree(double v, double ellipse)
{
    if (n_samples==0) return false;

    int16_t d = get_angle_bucket(v);
    if (data[d]==0)
    {
        data[d] = 1;
        tot++;
        scores[d] = (uint8_t)ellipse;
        score += angle_score(v);
        return true;
    }
    else
    {
        return false;
    }
}

int16_t Wind360::get_angle_bucket(double v) const
{
    if (n_samples==0) return -1;

    v = norm_deg(v + sample_size / 2.0); // center the sample within the bucket
    return (int16_t)(v / sample_size);
}

unsigned char Wind360::get_data(int ix) const
{
    if (n_samples==0 || ix<0 || ix>=buffer_size()) return 0;

    uint8_t c = 0;
    for (int i = 0; i<8; i++)
    {
        if (data[i + (ix * 8)]) c = c | (1 << i);
    }
    return c;
}

int16_t Wind360::buffer_size() const
{
    return n_samples / 8 + ((n_samples % 8)?1:0);
}

int16_t Wind360::size() const
{
    return n_samples;
}

int16_t Wind360::progress() const 
{ 
    return tot; 
} 

bool Wind360::is_valid() const
{
    return tot >= n_samples;
}

double Wind360::get_score() const
{
    return n_samples?(score / tot_score):0;
}
