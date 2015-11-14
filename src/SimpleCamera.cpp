//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "dxut\cmmn.h"
#include "dxut\SimpleCamera.h"

SimpleCamera::SimpleCamera():
	initialPosition(0, 0, 0),
	position(initialPosition),
	yaw(XM_PI),
	pitch(0.0f),
	lookDirection(0, 0, -1),
	upDirection(0, 1, 0),
	moveSpeed(20.0f),
	turnSpeed(XM_PIDIV2)
{
	ZeroMemory(&keysPressed, sizeof(keysPressed));
}

void SimpleCamera::Init(XMFLOAT3 position)
{
	initialPosition = position;
	Reset();
}

void SimpleCamera::SetMoveSpeed(float unitsPerSecond)
{
	moveSpeed = unitsPerSecond;
}

void SimpleCamera::SetTurnSpeed(float radiansPerSecond)
{
	turnSpeed = radiansPerSecond;
}

void SimpleCamera::Reset()
{
	position = initialPosition;
	yaw = XM_PI;
	pitch = 0.0f;
	lookDirection = { 0, 0, -1 };

	
}

void SimpleCamera::Update(float elapsedSeconds)
{
	// Calculate the move vector in camera space.
	XMFLOAT3 move(0, 0, 0);

	if (keysPressed.a)
		move.x -= 1.0f;
	if (keysPressed.d)
		move.x += 1.0f;
	if (keysPressed.w)
		move.z -= 1.0f;
	if (keysPressed.s)
		move.z += 1.0f;
	if (keysPressed.q)
		move.y += 1.f;
	if (keysPressed.e)
		move.y -= 1.f;

	if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f && fabs(move.y) > 0.1f)
	{
		XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
		move.x = XMVectorGetX(vector);
		move.z = XMVectorGetZ(vector);
		move.y = XMVectorGetY(vector);
	}

	float moveInterval = moveSpeed * elapsedSeconds;
	float rotateInterval = turnSpeed * elapsedSeconds;

	if (keysPressed.left)
		yaw += rotateInterval;
	if (keysPressed.right)
		yaw -= rotateInterval;
	if (keysPressed.up)
		pitch += rotateInterval;
	if (keysPressed.down)
		pitch -= rotateInterval;

	// Prevent looking too far up or down.
	pitch = min(pitch, XM_PIDIV2-0.1f);
	pitch = max(-XM_PIDIV2+0.1f, pitch);

	// Move the camera in model space.
	float x = move.x * -cosf(yaw) - move.z * sinf(yaw);
	float z = move.x * sinf(yaw) - move.z * cosf(yaw);
	position.x += x * moveInterval;
	position.z += z * moveInterval;
	position.y += move.y * moveInterval;

	// Determine the look direction.
	float r = cosf(pitch);
	lookDirection.x = r * sinf(yaw);
	lookDirection.y = sinf(pitch);
	lookDirection.z = r * cosf(yaw);
}

XMMATRIX SimpleCamera::GetViewMatrix()
{
	return XMMatrixLookToRH(XMLoadFloat3(&position), XMLoadFloat3(&lookDirection), XMLoadFloat3(&upDirection));
}

XMMATRIX SimpleCamera::GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	return XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane);
}

void SimpleCamera::OnKeyDown(WPARAM key)
{
	switch (key)
	{
	case 'W':
		keysPressed.w = true;
		break;
	case 'A':
		keysPressed.a = true;
		break;
	case 'S':
		keysPressed.s = true;
		break;
	case 'D':
		keysPressed.d = true;
		break;
	case 'Q':
		keysPressed.q = true;
		break;
	case 'E':
		keysPressed.e = true;
		break;
	case VK_LEFT:
		keysPressed.left = true;
		break;
	case VK_RIGHT:
		keysPressed.right = true;
		break;
	case VK_UP:
		keysPressed.up = true;
		break;
	case VK_DOWN:
		keysPressed.down = true;
		break;
	case VK_ESCAPE:
		Reset();
		break;
	}
}

void SimpleCamera::OnKeyUp(WPARAM key)
{
	switch (key)
	{
	case 'W':
		keysPressed.w = false;
		break;
	case 'A':
		keysPressed.a = false;
		break;
	case 'S':
		keysPressed.s = false;
		break;
	case 'D':
		keysPressed.d = false;
		break;
	case 'Q':
		keysPressed.q = false;
		break;
	case 'E':
		keysPressed.e = false;
		break;
	case VK_LEFT:
		keysPressed.left = false;
		break;
	case VK_RIGHT:
		keysPressed.right = false;
		break;
	case VK_UP:
		keysPressed.up = false;
		break;
	case VK_DOWN:
		keysPressed.down = false;
		break;
	}
}
