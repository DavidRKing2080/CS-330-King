///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free up the allocated memory
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}
	// clear the collection of defined materials
	m_objectMaterials.clear();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}

}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/green_apple.jpg",
		"apple");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/apple_stem.jpg",
		"stem");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg",
		"table");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/ceramic1.jpg",
		"ceramic");

	bReturn = CreateGLTexture(
		"../../Utilities/textures/white_cardboard.jpg",
		"cardboard");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
	goldMaterial.shininess = 22.0;
	goldMaterial.tag = "gold";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL appleMaterial;
	appleMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
	appleMaterial.ambientStrength = 0.2f;
	appleMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	appleMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	appleMaterial.shininess = 5.0;
	appleMaterial.tag = "appleskin";

	m_objectMaterials.push_back(appleMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL polishwoodMaterial;
	polishwoodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	polishwoodMaterial.ambientStrength = 0.2f;
	polishwoodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	polishwoodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	polishwoodMaterial.shininess = 11.0;
	polishwoodMaterial.tag = "polishWood";

	m_objectMaterials.push_back(polishwoodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.2f, 0.3f, 0.4f);
	tileMaterial.ambientStrength = 0.3f;
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.4f, 0.5f, 0.6f);
	tileMaterial.shininess = 25.0;
	tileMaterial.tag = "tile";

	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL polishclayMaterial;
	polishclayMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	polishclayMaterial.ambientStrength = 0.2f;
	polishclayMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	polishclayMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	polishclayMaterial.shininess = 30.0;
	polishclayMaterial.tag = "polishClay";

	m_objectMaterials.push_back(polishclayMaterial);
}

void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

	m_pShaderManager->setVec3Value("lightSources[3].position", -0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.5f);

	m_pShaderManager->setBoolValue("bUseLighting", true);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation2;
	glm::mat4 translation;
	glm::mat4 model;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	/****************************************************************/
	//Table plane
	/****************************************************************/

	// Declare the variables for the table transformations
	glm::vec3 newPlaneScaleXYZ = glm::vec3(20.0f, 0.0f, 10.0f); // Same scale as the original plane
	float newPlaneXrotationDegrees = 0.0f;
	float newPlaneYrotationDegrees = 0.0f;
	float newPlaneZrotationDegrees = 0.0f;
	glm::vec3 newPlanePositionXYZ = glm::vec3(0.0f, 1.5f, 0.0f); // Adjusted position above the original plane

	// Set transformations for the new plane
	SetTransformations(
		newPlaneScaleXYZ,
		newPlaneXrotationDegrees,
		newPlaneYrotationDegrees,
		newPlaneZrotationDegrees,
		newPlanePositionXYZ);

	// Set color for the new plane (light brown/beige)
	SetShaderColor(0.9f, 0.8f, 0.7f, 1.0f); // Light brown/beige color

	// Draw the new plane

	SetShaderMaterial("polishWood");
	SetShaderTexture("table");
	m_basicMeshes->DrawPlaneMesh();


	/****************************************************************/
	//Apple and stem
	/****************************************************************/

	// Sphere for apple
	// Declare the variables for the apple transformations
	glm::vec3 appleScaleXYZ = glm::vec3(1.0f, 1.1f, 1.0f); // Slightly flattened sphere
	float appleXrotationDegrees = 0.0f;
	float appleYrotationDegrees = 0.0f;
	float appleZrotationDegrees = -1.0f;
	glm::vec3 applePositionXYZ = glm::vec3(0.0f, 2.5f, 0.0f); // Adjusted position

	// Set the transformations for the apple
	SetTransformations(
		appleScaleXYZ,
		appleXrotationDegrees,
		appleYrotationDegrees,
		appleZrotationDegrees,
		applePositionXYZ);

	// Set the color for the apple (green)
	//SetShaderColor(0.55f, 0.71f, 0.0f, 1.0f); // Green apple color

	// Draw the apple (sphere mesh)

	SetShaderMaterial("appleskin");
	SetShaderTexture("apple");
	m_basicMeshes->DrawSphereMesh(); // Draw apple-shaped sphere

	/****************************************************************/

   // Cylinder for apple stem
   // Declare the variables for the stem transformations
	glm::vec3 stemScaleXYZ = glm::vec3(0.1f, 1.0f, 0.1f); // Thin and tall cylinder
	float stemXrotationDegrees = 0.0f;
	float stemYrotationDegrees = 0.0f;
	float stemZrotationDegrees = -15.0f;
	glm::vec3 stemPositionXYZ = glm::vec3(0.0f, 3.0f, 0.0f); // Positioned on top of the apple

	// Set the transformations for the stem
	SetTransformations(
		stemScaleXYZ,
		stemXrotationDegrees,
		stemYrotationDegrees,
		stemZrotationDegrees,
		stemPositionXYZ);

	// Set the color for the stem (brown)
	//SetShaderColor(0.55f, 0.27f, 0.07f, 1.0f); // Brown color

	// Draw the stem (cylinder mesh)

	SetShaderMaterial("wood");
	SetShaderTexture("stem");
	m_basicMeshes->DrawCylinderMesh(); // Draw stem-shaped cylinder

	/****************************************************************/
	//Ceremic container
	//4 parts: Container, lid, lid2, and lid3
	/****************************************************************/

	//Cylinder for container
	// Declare the variables for the container transformations
	glm::vec3 containerScaleXYZ = glm::vec3(1.5f, 2.0f, 1.5f); // Slightly taller, wider cylinder
	float containerXrotationDegrees = 0.0f;
	float containerYrotationDegrees = 0.0f;
	float containerZrotationDegrees = 0.0f;
	glm::vec3 containerPositionXYZ = glm::vec3(2.5f, 1.5f, 0.0f); // Adjusted position to be next to apple

	// Set the transformations for the container
	SetTransformations(
		containerScaleXYZ,
		containerXrotationDegrees,
		containerYrotationDegrees,
		containerZrotationDegrees,
		containerPositionXYZ);

	//SetShaderColor(0.55f, 0.71f, 0.0f, 1.0f); // Green apple color

	// Draw the apple (sphere mesh)

	SetShaderMaterial("polishClay");
	SetShaderTexture("ceramic");
	m_basicMeshes->DrawCylinderMesh(); // Draw cylinder

	//Sphere for container lid top
// Declare the variables for the container transformations
	glm::vec3 lidScaleXYZ = glm::vec3(0.5f, 0.325f, 0.5f); // Slightly wider sphere
	float lidXrotationDegrees = 0.0f;
	float lidYrotationDegrees = 0.0f;
	float lidZrotationDegrees = 0.0f;
	glm::vec3 lidPositionXYZ = glm::vec3(2.5f, 3.85f, 0.0f); //Positioned to rest on container

	// Set the transformations for lid
	SetTransformations(
		lidScaleXYZ,
		lidXrotationDegrees,
		lidYrotationDegrees,
		lidZrotationDegrees,
		lidPositionXYZ);

	//SetShaderColor(0.55f, 0.71f, 0.0f, 1.0f); // Green apple color

	// Draw the apple (sphere mesh)

	SetShaderMaterial("polishClay");
	SetShaderTexture("ceramic");
	m_basicMeshes->DrawSphereMesh(); // Draw sphere

	//Torus for lid2
	// Declare the variables for the container transformations
	glm::vec3 lid2ScaleXYZ = glm::vec3(1.25f, 1.25f, 1.25f); // Torus to fit at top of container
	float lid2XrotationDegrees = 90.0f;
	float lid2YrotationDegrees = 0.0f;
	float lid2ZrotationDegrees = 0.0f;
	glm::vec3 lid2PositionXYZ = glm::vec3(2.5f, 3.45f, 0.0f); // Adjusted to be radial around container top edge

	// Set the transformations for lid2
	SetTransformations(
		lid2ScaleXYZ,
		lid2XrotationDegrees,
		lid2YrotationDegrees,
		lid2ZrotationDegrees,
		lid2PositionXYZ);

	//SetShaderColor(0.55f, 0.71f, 0.0f, 1.0f); // Green apple color

	// Draw the apple (sphere mesh)

	SetShaderMaterial("polishClay");
	SetShaderTexture("ceramic");
	m_basicMeshes->DrawTorusMesh(); // Draw sphere

	//Cylinder for lid3
// Declare the variables for the container transformations
	glm::vec3 lid3ScaleXYZ = glm::vec3(1.25f, 0.25f, 1.25f); // Torus to fit at top of container
	float lid3XrotationDegrees = 0.0f;
	float lid3YrotationDegrees = 0.0f;
	float lid3ZrotationDegrees = 0.0f;
	glm::vec3 lid3PositionXYZ = glm::vec3(2.5f, 3.45f, 0.0f); // Adjusted to fill space inside the torus

	// Set the transformations for lid3
	SetTransformations(
		lid3ScaleXYZ,
		lid3XrotationDegrees,
		lid3YrotationDegrees,
		lid3ZrotationDegrees,
		lid3PositionXYZ);

	//SetShaderColor(0.55f, 0.71f, 0.0f, 1.0f); // Green apple color


	SetShaderMaterial("polishClay");
	SetShaderTexture("ceramic");
	m_basicMeshes->DrawCylinderMesh(); // Draw cylinder

	/****************************************************************/
	//Box 1
	/****************************************************************/

	//Box for box1
	// Declare the variables for the container transformations
	glm::vec3 box1ScaleXYZ = glm::vec3(4.0f, 3.0f, 3.0f); // Torus to fit at top of container
	float box1XrotationDegrees = 0.0f;
	float box1YrotationDegrees = -30.0f;
	float box1ZrotationDegrees = 0.0f;
	glm::vec3 box1PositionXYZ = glm::vec3(2.5f, 3.0f, -3.5f); // Adjusted to fill space inside the torus

	// Set the transformations for the box
	SetTransformations(
		box1ScaleXYZ,
		box1XrotationDegrees,
		box1YrotationDegrees,
		box1ZrotationDegrees,
		box1PositionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // Regular white color

	SetShaderMaterial("wood");
	SetShaderTexture("cardboard");
	m_basicMeshes->DrawBoxMesh(); // Draw box1

	/****************************************************************/
	//Box 2
	/****************************************************************/

	//Box for box2
	// Declare the variables for the container transformations
	glm::vec3 box2ScaleXYZ = glm::vec3(2.5f, 3.25f, 0.5f); // Tall, thin, wide box
	float box2XrotationDegrees = 0.0f;
	float box2YrotationDegrees = 25.0f;
	float box2ZrotationDegrees = 0.0f;
	glm::vec3 box2PositionXYZ = glm::vec3(-0.35f, 3.25f, -1.5f); // Behind apple

	// Set the transformations for the box
	SetTransformations(
		box2ScaleXYZ,
		box2XrotationDegrees,
		box2YrotationDegrees,
		box2ZrotationDegrees,
		box2PositionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // Regular white color

	SetShaderMaterial("wood");
	SetShaderTexture("cardboard");
	m_basicMeshes->DrawBoxMesh(); // Draw box2

	/****************************************************************/
	//Teacup
	/****************************************************************/

	//Tapered Cylinder for teacup
	glm::vec3 teacupScaleXYZ = glm::vec3(1.5f, 1.5f, 1.5f); // Tall, thin, wide teacup
	float teacupXrotationDegrees = 180.0f;
	float teacupYrotationDegrees = 0.0f;
	float teacupZrotationDegrees = 0.0f;
	glm::vec3 teacupPositionXYZ = glm::vec3(2.5f, 6.0f, -3.5f); // On top of box1

	// Set the transformations for the teacup
	SetTransformations(
		teacupScaleXYZ,
		teacupXrotationDegrees,
		teacupYrotationDegrees,
		teacupZrotationDegrees,
		teacupPositionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // Regular white color

	SetShaderMaterial("polishClay");
	SetShaderTexture("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh(); // Draw teacup

	//Torus for teacup handle
	glm::vec3 handleScaleXYZ = glm::vec3(0.5f, 0.5f, 0.5f); // Tall, thin, wide handle
	float handleXrotationDegrees = 0.0f;
	float handleYrotationDegrees = 0.0f;
	float handleZrotationDegrees = 0.0f;
	glm::vec3 handlePositionXYZ = glm::vec3(3.5f, 5.25f, -3.5f); // On top of box1

	// Set the transformations for the handle
	SetTransformations(
		handleScaleXYZ,
		handleXrotationDegrees,
		handleYrotationDegrees,
		handleZrotationDegrees,
		handlePositionXYZ);

	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f); // Regular white color

	SetShaderMaterial("polishClay");
	SetShaderTexture("ceramic");
	m_basicMeshes->DrawTorusMesh(); // Draw handle
}

