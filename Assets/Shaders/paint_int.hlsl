
#pragma ps main

#include "common.hlsli"

cbuffer CodeConstants : register(CodeConstantsSlot)
{
    int toPaint;
};

int main() : SV_TARGET
{
    return toPaint;
}